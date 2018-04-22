//=============================================================================
//  端末のフォアグラウンドプロセスのPIDを取得する
//=============================================================================
#define _POSIX_SOURCE
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>

int main()
{
    pid_t pid;
    int fd;

    if ((fd = open("/dev/tty", O_RDONLY)) < 0) {
        perror("open /dev/tty error");
    }

    if ((pid = tcgetpgrp(fd)) < 0)
        perror("tcgetpgrp() error");
    else
        printf("the foreground process group id of /dev/tty is %d\n", (int) pid);

    return 0;
}
