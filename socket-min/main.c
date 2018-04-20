//=============================================================================
//  本当に最小限のechoサーバ（ポート9000）
//  エラーチェックすらなし
//=============================================================================
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
    int sock;
    int accepted;
    struct sockaddr_in address;
    char buf[256];
    int nread;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(9000);
    bind(sock, (struct sockaddr*)&address, sizeof(struct sockaddr_in));

    listen(sock, 5);

    while (1) {
        accepted = accept(sock, NULL, NULL);

        while ((nread = read(accepted, buf, sizeof(buf)))) {
            for (int i = 0; i < nread; i++)
                buf[i] = toupper(buf[i]);
            write(accepted, buf, nread);
        }
        close(accepted);
    }

    return 0;
}
