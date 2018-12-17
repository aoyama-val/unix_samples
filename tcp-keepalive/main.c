//=============================================================================
//  TCP Keep-aliveなソケットを作るサンプル
//  keep-aliveパケットに応答が無いとTCPコネクションが切断され、
//  （おそらくread()のところで）SIGPIPEが発生する。
//=============================================================================
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>

void set_keepalive(int s)
{
    int enable  =  1; /* set keepalive on/off */
    int idle    = 10; /* idle time used when SO_KEEPALIVE is enabled */
    int intvl   = 5; /* interval between keepalives */
    int keepcnt =  2; /* number of keepalives before close */

    if (setsockopt(s, SOL_SOCKET,  SO_KEEPALIVE,  &enable,  sizeof(enable))) {
        perror("SO_KEEPALIVE");
        exit(1);
    }
    if (setsockopt(s, IPPROTO_TCP, TCP_KEEPIDLE,  &idle,    sizeof(idle))) {
        perror("TCP_KEEPIDLE");
        exit(1);
    }
    if (setsockopt(s, IPPROTO_TCP, TCP_KEEPINTVL, &intvl,   sizeof(intvl))) {
        perror("TCP_KEEPINTVL");
        exit(1);
    }
    if (setsockopt(s, IPPROTO_TCP, TCP_KEEPCNT,   &keepcnt, sizeof(keepcnt))) {
        perror("TCP_KEEPCNT");
        exit(1);
    }
}

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

    set_keepalive(s);

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
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    error = getnameinfo(res->ai_addr, res->ai_addrlen, hbuf,
                        sizeof(hbuf), sbuf, sizeof(sbuf),
                        NI_NUMERICHOST | NI_NUMERICSERV);
    if (error) {
        fprintf(stderr, "%s\n", gai_strerror(error));
        exit(1);
    }
    fprintf(stderr, "listen to %s %s\n", hbuf, sbuf);

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

void signal_handler(int n)
{
    char * s = "caught signal!\n";
    write(1, s, strlen(s));
    exit(0);
}

void set_sighandler()
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signal_handler;

    if (sigaction(SIGPIPE, &sa, NULL) != 0) {
        perror("sigaction");
    }
}

int main(int argc, char* argv[])
{
    int af = AF_INET;
    int listening;
    int accepted;
    char buf[256];
    int nread = 0;
    struct sockaddr_storage from;
    socklen_t fromlen;

    set_sighandler();

    af = AF_INET;

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
        while ((nread = read(accepted, buf, sizeof(buf)))) {
            write(accepted, "hello\n", 6);
        }
        close(accepted);
        printf("closed\n");
    }
}
