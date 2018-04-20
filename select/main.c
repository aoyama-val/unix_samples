#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

int main(int argc, char *argv[])
{
    int fd1 = open("fifo1", O_RDONLY | O_NONBLOCK);
    int fd2 = open("fifo2", O_RDONLY | O_NONBLOCK);

    if (fd1 < 0 || fd2 < 0) {
        err(1, "open");
    }

    // int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
    //
    // nfds は 3 つの集合に含まれるファイルディスクリプターの最大値に 1 を足したものである。
    //
    // 成功した場合、 select() と pselect() は更新された 3 つのディスクリプター集合に含まれている
    // ファイルディスクリプターの数 (つまり、 readfds, writefds, exceptfds 中の 1 になっているビットの総数) を返す。 
    // 何も起こらずに時間切れになった場合、 ディスクリプターの数は 0 になることもある。 
    // エラーならば -1 を返し、 errno にエラーを示す値が設定される; ファイルディスクリプター集合は変更されず、 timeout は不定となる。  

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd1, &readfds);
    FD_SET(fd2, &readfds);

    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    printf("select timeout = %d\n", tv.tv_sec);
    int n = select(fd2 + 1, &readfds, NULL, NULL, &tv);
    if (n < 0) {
        err(1, "select");
    }
    printf("n = %d\n", n);
    if (FD_ISSET(fd1, &readfds)) {
        printf("fd1 readable\n");
    }
    if (FD_ISSET(fd2, &readfds)) {
        printf("fd2 readable\n");
    }

    close(fd1);
    close(fd2);

    return 0;
}
