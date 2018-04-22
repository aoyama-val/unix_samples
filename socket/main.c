/*
 * 接続してきたクライアントに対して文字列を送信するサーバ
 * IPv6/IPv4デュアルバージョン
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
    int s[64]; /* クライアントと通信を行うソケット */
    struct addrinfo hints; /* getaddrinfoの動作を指定するヒント情報 */
    struct addrinfo *ai0; /* addrinfoリストの先頭の要素*/
    struct addrinfo *ai; /* 処理中のaddrinfoリストの要素 */
    int ret; /* getaddrinfoの処理結果 */
    char *servname; /* ポート名 */
    int smax; /* 接続を受け入れるプロトコルの数 */
    fd_set rfd, rfd0;
    int i;

    /** コマンドライン引数のチェック */
    if (argc != 2) {
        fprintf(stderr, "usage: dual-tcp-server <port>\n");
        exit(1);
    }

    /** コマンドラインの第一引数（ポート名）をセット */
    servname = argv[1];
    /** ヒント情報の初期化 */
    memset(&hints, 0, sizeof(hints));
    /* プロトコルは指定しない: IPv4 or IPv6 */
    /* ストリーム型(TCP)による通信を指定 */
    hints.ai_socktype = SOCK_STREAM;
    /* PASSIVE型のソケット生成を指定 */
    hints.ai_flags = AI_PASSIVE;
    /* 名前とポート名の解決　＆　接続候補のリストを取得 */
    ret = getaddrinfo(NULL, servname, &hints, &ai0);
    if (ret) {
        fprintf(stderr, "%s", gai_strerror(ret));
        exit(1);
    }
    smax = 0;
    printf("AF_INET = %d\n", AF_INET);
    printf("AF_INET6 = %d\n", AF_INET6);
    /** addrinfoリストの要素を先頭から接続できるまで順に試行する */
    for (ai = ai0; ai; ai = ai->ai_next) {
        if (ai->ai_family != AF_INET6)
            continue;
        printf("ai_family = %d ai_socktype = %d ai_protocol = %d\n", ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        /* ソケットの生成 */
        s[smax] = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        /* ソケットの生成に失敗したらリストの次の項目を試す */
        if (s[smax] < 0)
            continue;
        /* ポート番号をソケットに結びつける */
        if (bind(s[smax], ai->ai_addr, ai->ai_addrlen) < 0) {
            perror("bind");
            close(s[smax]);
            continue;
        }
        /* クライアントからの接続の要求へ待機する */
        if (listen(s[smax], 5) < 0) {
            perror("listen");
            close(s[smax]); 
            continue;
        }
        smax++;
    }
    printf("smax = %d\n", smax);
    /** forループで待受けソケットを生成できない場合はエラーとする */
    if (smax == 0) {
        fprintf(stderr, "cannot create server socket\n");
        exit(1);
    }
    /** s[xx]へのクライアントからの接続要求をselectにより監視 */
    FD_ZERO(&rfd0);
    for (i = 0; i < smax; ++i)
        FD_SET(s[i], &rfd0);
    /** クライアントとのI/O */
    while (1) {
        int as;
        struct sockaddr_storage ss; /* クライアントのソケット アドレス */
        unsigned int sslen; /* ソケットアドレスの長さ */
        int n;
        int pid;
        /* クライアントが接続してくるまでselect()で待つ */
        rfd = rfd0;
        printf("before select\n");
        n = select(s[smax-1] + 1, &rfd, NULL, NULL, NULL);
        printf("select\n");
        if (n < 0) {
            perror("select");
            exit(1);
        }
        /* 接続を受け付けたソケットを求める */
        for (i = 0; i < smax; ++i) {
            if (FD_ISSET(s[i], &rfd))
                break;
        }
        /* クライアントからの接続を受け入れる際に
           クライアントを示すソケットアドレスがssに書き込まれている */
        printf("i = %d s[i] = %d\n", i, s[i]);
        as = accept(s[i], (struct sockaddr *)&ss, &sslen);
        printf("accepd\n");
        if (as < 0) {
            perror("accept");
            exit(1);
        }
        {
            char buf[256];
            FILE* fp = fdopen(as, "r+");
            int i = 0;
            memset(buf, 0, sizeof(buf));
            while (fgets(buf, sizeof(buf) - 1, fp)) {
                printf("read[%d] [%s]\n", i, buf);
                memset(buf, 0, sizeof(buf));

                printf("writing\n");
                char *text = "send string: client!\n";
                fprintf(fp, "%s", text);
                fflush(fp);
                i++;
            }
            printf("close\n");
            /* サブプロセスの処理 */
            fclose(fp);
            //write(as, text, strlen(text));
            //close(as);
        }
    }
}
