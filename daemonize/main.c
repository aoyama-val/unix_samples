#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

#define MY_DAEMONIZE    0   // daemon(3)を使用
//#define MY_DAEMONIZE    1   // 自前の実装を使用

// 2回forkする流儀もあるが、これは1回だけ。
void daemonize()
{
    switch (fork()) {
        case -1:
            err(1, "fork");
            break;
        case 0:
            break;
        default:
            exit(0);
            break;
    }

    if (setsid() == -1) {
        err(1, "setsid");
    }

    pid_t pid = getpid();
    printf("pid = %ld\n", (long)pid);

    int fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        err(1, "open");
    }

    if (dup2(fd, 0) < 0) {
        err(1, "dup2");
    }
    if (dup2(fd, 1) < 0) {
        err(1, "dup2");
    }
    if (dup2(fd, 2) < 0) {
        err(1, "dup2");
    }
    if (fd > 2) {
        close(fd);
    }

    chdir("/");
}

int main(int argc, char *argv[])
{
    FILE* fp = fopen("log.txt", "w");
    if (fp == NULL) {
        err(1, "open");
    }

#if MY_DAEMONIZE
    daemonize();
#else
    daemon(0, 0);
#endif

    while (1) {
        fprintf(fp, "running %ld\n", (long)getpid());
        fflush(fp);
        sleep(1);
    }
    return 0;
}
