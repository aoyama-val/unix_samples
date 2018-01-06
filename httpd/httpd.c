#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>

enum model {
    MODEL_FORK    = 0,
    MODEL_THREAD  = 1,
    MODEL_SINGLE  = 2,
    MODEL_PREFORK = 3,     // 今のところ、2プロセスだけの手抜き実装
};

struct thread_arg {
    int connected_socket;
    int listening_socket;
};

void parse_request_line(char* line, char* method, char* path, char* version)
{
    sscanf(line, "%s %s %s", method, path, version);
}

char* load_response(char* request_path)
{
    struct stat sb;
    char path[256] = ".";

    strncat(path, request_path, sizeof(path) - 1);

    if (stat(path, &sb) != 0 || !(sb.st_mode & S_IFREG)) {
        return NULL;
    }
    FILE* fp = fopen(path, "r");
    if (fp == NULL)
        return NULL;
    char* content = malloc(sb.st_size);
    fread(content, 1, sb.st_size, fp);
    fclose(fp);
    return content;
}

int process_request(int connected_socket)
{
    char buf[4096];
    char method[16];
    char path[256];
    char version[16];

    FILE* fp = fdopen(connected_socket, "r+b");
    if (fp == NULL) {
        perror("fdopen");
        exit(1);
    }

    setvbuf(fp, NULL, _IONBF, 0);

    int lnum = 0;
    while (fgets(buf, sizeof(buf), fp)) {
        lnum++;
        if (lnum == 1) {
            parse_request_line(buf, method, path, version);
            printf("method = [%s]  path = [%s]  version = [%s]\n", method, path, version);
        }
        if (strcmp(buf, "\r\n"))
            break;
    }

    char* res = load_response(path);
    if (res == NULL) {
        char* not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\nNot Found";
        fwrite(not_found, 1, strlen(not_found), fp);

    } else {
        char header[1024];
        int len = strlen(res);
        sprintf(header, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", len);
        fwrite(header, 1, strlen(header), fp);
        fwrite(res, 1, len, fp);
        free(res);
    }

    fclose(fp);

    return 0;
}

void* thread_func(void* data)
{
    struct thread_arg arg = *(struct thread_arg*)data;
    process_request(arg.connected_socket);
    return NULL;
}

void sigchld_handler()
{
    // errnoを復元するために保存しておく。
    // errnoはスレッドローカルなので、マルチスレッドでも大丈夫。
    int saved_errno = errno;

    // SIGCHLDハンドラ実行中にSIGCHLDが複数回生成された場合、
    // 1個のみが配送される。そのため、ループを回して終了している子プロセスを
    // 全部waitする必要がある。そうしないとゾンビが出来てしまう。

    int status;
    pid_t pid;
    // WNOHANGにより、終了している子プロセスが無い場合はブロックせずにすぐに戻り、戻り値は0になる。
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        continue;
    }
    errno = saved_errno;
}

void set_signal_handler()
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler; // 単に子プロセスをwaitするだけなら、SIG_IGNをセットすれば自動的にやってくれるのでもっと楽。
    sa.sa_flags = SA_RESTART;   // システムコールを再開させる。これをつけないとacceptが中断されてしまってすごく遅かった。
    sigaction(SIGCHLD, &sa, NULL);
}

// ソケットの相手の情報を表示する
void get_peer_address(int sock, char buf[])
{
    socklen_t len;
    struct sockaddr_storage addr;
    char ipstr[INET6_ADDRSTRLEN];
    int port;

    len = sizeof(addr);
    getpeername(sock, (struct sockaddr*)&addr, &len);

    // deal with both IPv4 and IPv6:
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
    } else { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof(ipstr));
    }

    sprintf(buf, "%s:%d", ipstr, port);
}

int main(int argc, char *argv[])
{
    int connected_socket, listening_socket;
    struct sockaddr_in sin;
    int len, ret;
    int sock_optval = 1;
    int port = 5000;
    enum model model = MODEL_FORK;

    pthread_t worker;

    if (argc > 1) {
        if (strcmp(argv[1], "fork") == 0) {
            model = MODEL_FORK;
            printf("model = fork\n");
        } else if (strcmp(argv[1], "thread") == 0) {
            model = MODEL_THREAD;
            printf("model = thread\n");
        } else if (strcmp(argv[1], "single") == 0) {
            model = MODEL_SINGLE;
            printf("model = single\n");
        } else if (strcmp(argv[1], "prefork") == 0) {
            model = MODEL_PREFORK;
            printf("model = prefork\n");
        }
    } else {
        model = MODEL_FORK;
        printf("model = fork\n");
    }

    // forkの前に大量メモリ確保すると遅くなることの確認用
    if (argc > 2) {
        int len = atoi(argv[2]);
        int bytes = sizeof(int) * len;
        int *large = malloc(bytes);
        if (large == NULL) {
            err(1, "malloc");
        }
        printf("malloc %d bytes\n", bytes);

        // mallocしただけではカーネルは実際にメモリを確保しないかもしれないので、アクセスしておく
        srand(time(NULL));
        for (int i = 0; i < len; i++) {
            large[i] = rand();
        }
    }

    set_signal_handler();

    /* リスニングソケットを作成 */
    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket == -1) {
        err(1, "socket");
    }
    /* ソケットオプション設定 */
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR,
                   &sock_optval, sizeof(sock_optval)) == -1) {
        err(1, "setsockopt");
    }
    /* アドレスファミリ・ポート番号・IPアドレス設定 */
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    /* ソケットにアドレス(＝名前)を割り付ける */
    if (bind(listening_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        err(1, "bind");
    }
    /* ポートを見張るよう、OS に命令する */
    ret = listen(listening_socket, SOMAXCONN);
    if (ret == -1) {
        err(1, "listen");
    }
    printf("ポート %d を見張ります。\n", port);

    if (model == MODEL_PREFORK) {
        fork();
    }

    while (1) {
        struct sockaddr_in peer_sin;
        len = sizeof(peer_sin);
        /* コネクション受け付け */
        connected_socket = accept(listening_socket, (struct sockaddr *)&peer_sin, (socklen_t*)&len);
        if (connected_socket == -1) {
            err(1, "accept");
        }

        //char peer_address[256];
        //get_peer_address(connected_socket, peer_address);
        //printf("接続: %s\n", peer_address);

        switch (model) {
            case MODEL_FORK:
                if (fork() == 0) {
                    //large[1] = rand();
                    close(listening_socket);
                    process_request(connected_socket);
                    exit(0);
                }
                close(connected_socket);
                break;
            case MODEL_THREAD:
                {
                    struct thread_arg* arg = malloc(sizeof(struct thread_arg));
                    arg->connected_socket = connected_socket;
                    arg->listening_socket = listening_socket;
                    if (pthread_create(&worker, NULL, thread_func, arg) != 0) {
                        err(1, "pthread_create");
                    }
                }
                break;
            case MODEL_SINGLE:
            case MODEL_PREFORK:
                {
                    process_request(connected_socket);
                }
                break;
            default:
                printf("Invalid model: %d\n", model);
                exit(1);
        }
    }
    ret = close(listening_socket);
    if (ret == -1) {
        err(1, "close");
    }

    sleep(10);

    return 0;
}
