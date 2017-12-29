//=============================================================================
//   ipv4, ipv6両対応のTCPサーバ
//   （デュアルスタックではなく、切り替え式）
//=============================================================================

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

/**
 * リスニングソケットを作成する。
 *
 * @param address_family    AF_INET / AF_INET6
 * @param port              ex. "80"
 * @param backlog           ex. 5
 */
int tcp_listen(int address_family, char* port, int backlog)
{
    struct addrinfo hints;
    struct addrinfo* res;
    int error;
    int s;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = address_family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    error = getaddrinfo(NULL, port, &hints, &res);
    if (error) {
        fprintf(stderr, "%s: %s\n", port, gai_strerror(error));
        exit(1);
    }
    if (res->ai_next) {
        fprintf(stderr, "%s: multiple address returned\n", port);
        exit(1);
    }
    s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s < 0) {
        perror("socket");
        exit(1);
    }
#ifdef IPV6_V6ONLY
    const int on = 1;
    if (res->ai_family == AF_INET6 &&
        setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) < 0) {
        perror("bind");
        exit(1);
    }
#endif
    if (bind(s, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(s, backlog) < 0) {
        perror("listen");
        exit(1);
    }

    // リスニングソケットのホスト名、ポートを文字列として取得する
    //char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    //error = getnameinfo(res->ai_addr, res->ai_addrlen, hbuf,
                        //sizeof(hbuf), sbuf, sizeof(sbuf),
                        //NI_NUMERICHOST | NI_NUMERICSERV);
    //if (error) {
        //fprintf(stderr, "%s\n", gai_strerror(error));
        //exit(1);
    //}
    //fprintf(stderr, "listen to %s %s\n", hbuf, sbuf);

    return s;
}

/**
 * acceptしたソケットから通信相手のIPアドレスとポート番号を取得する。
 */
void get_peer_address(int sock, char strhost[], char strport[])
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

    strcpy(strhost, ipstr);
    sprintf(strport, "%d", port);
}

int main(int argc, char* argv[])
{
    int ch;
    int af = AF_INET;
    int listening;
    int accepted;
    struct sockaddr_storage from;
    socklen_t fromlen;

    while ((ch = getopt(argc, argv, "46")) != -1) {
        switch (ch) {
            case '4':
                af = AF_INET;
                break;
            case '6':
                af = AF_INET6;
                break;
            default:
                fprintf(stderr, "usage: test [-46]\n");
                exit(1);
        }
    }
    argc -= optind;
    argv += optind;

    listening = tcp_listen(af, "4000", 5);

    while (1) {
        fromlen = sizeof(from);
        accepted = accept(listening, (struct sockaddr *)&from, &fromlen);
        if (accepted < 0)
            continue;
        char strhost[256];
        char strport[256];
        get_peer_address(accepted, strhost, strport);
        printf("accepted: %s:%s\n", strhost, strport);
        write(accepted, "hello\n", 6);
        close(accepted);
    }
}
