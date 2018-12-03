//=============================================================================
//  socketpair()のサンプル
//  socketpairとはpipeのようなもの
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int socks[2];
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socks) != 0) {
        perror("socketpair");
        exit(1);
    }

    switch (fork()) {
        case 0:
            {
                char buf[100];
                write(socks[1], "hoge\n", 5);
                int nread = read(socks[0], buf, sizeof(buf));
                if (nread < 0) {
                    perror("read");
                }
                buf[nread] = '\0';
                printf("[%s]\n", buf);
                return 1;
            }
            break;
        case -1:
            perror("fork");
            exit(1);
            break;
        default:
            {
                char buf[256];
                int nread = read(socks[0], buf, sizeof(buf));
                if (nread < 0) {
                    perror("read");
                }
                buf[nread] = '\0';
                printf("[%s]\n", buf);
                write(socks[1], "peso", 4);
            }
            break; 
    }

    return 0;
}
