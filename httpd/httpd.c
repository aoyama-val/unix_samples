#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <pthread.h>
#include <wait.h>

#define MODEL_FORK      0
#define MODEL_THREAD    1

char *response = "";

struct thread_arg {
    int connected_socket;
    int listening_socket;
};

char* load_response(char* filename)
{
    char* ret = malloc(1024 * 10);
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        err(1, "fopen");
    }
    int nread = fread(ret, 1, 1024 * 10, fp);
    printf("loaded %d bytes\n", nread);
    ret[nread] = '\0';
    return ret;
}

void process_request(int connected_socket)
{
    int read_size;
    char buf[256];

    while (read(connected_socket, buf, sizeof(buf)) < 0)
        ;

    while (write(connected_socket, response, strlen(response)) < 0)
        ;

    printf("接続が切れました。子プロセス %d を終了します。\n", (int)getpid());
    int ret = close(connected_socket);
    if (ret == -1) {
        err(1, "close");
    }
}

void* thread_func(void* data)
{
    struct thread_arg arg = *(struct thread_arg*)data;
    process_request(arg.connected_socket);
    return NULL;
}

void sigchld_handler()
{
    wait(NULL);
}

void set_signal_handler()
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sa, NULL);
}

int main(int argc, char *argv[])
{
    int connected_socket, listening_socket;
    struct sockaddr_in sin;
    int len, ret;
    int sock_optval = 1;
    int port = 5000;
    int model = MODEL_FORK;

    pthread_t worker;

    if (argc > 1) {
        if (strcmp(argv[1], "fork") == 0) {
            model = MODEL_FORK;
            printf("model = fork\n");
        } else if (strcmp(argv[1], "thread") == 0) {
            model = MODEL_THREAD;
            printf("model = thread\n");
        }
    }

    set_signal_handler();

    response = load_response("response.txt");

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

    while (1) {
        struct sockaddr_in peer_sin;
        len = sizeof(peer_sin);
        /* コネクション受け付け */
        while (1) {
            connected_socket = accept(listening_socket, (struct sockaddr *)&peer_sin, (socklen_t*)&len);
            if (connected_socket == -1) {
                if (errno != EINTR)
                    err(1, "accept");
            }
            break;
        }

        /* 相手側のホスト・ポート情報を表示 */
        struct hostent* peer_host = gethostbyaddr((char *)&peer_sin.sin_addr.s_addr,
                                                  sizeof(peer_sin.sin_addr), AF_INET);
        if (peer_host == NULL) {
            printf("gethostbyname failed\n");
            exit(1);
        }

        printf("接続: %s [%s] ポート %d\n",
               peer_host->h_name,
               inet_ntoa(peer_sin.sin_addr),
               ntohs(peer_sin.sin_port)
              );


        switch (model) {
            case MODEL_FORK:
                if (fork() == 0) {
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
            default:
                printf("unknown model: %d\n", model);
                exit(1);
        }
    }
    ret = close(listening_socket);
    if (ret == -1) {
        err(1, "close");
    }

    return 0;
}
