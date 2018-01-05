//=============================================================================
//   引数で指定された個数のプロセスをfork()しておき、
//   スリープするだけのサンプル。
//   子プロセスが死んだら自動的に再fork()する。
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <err.h>
#include <time.h>

void millisleep(int ms)
{
    struct timespec req;
    req.tv_sec = 0;
    req.tv_nsec = 1000 * 1000 * ms;
    nanosleep(&req, NULL);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s PROCESS_COUNT\n", argv[0]);
        exit(1);
    }

    int process_count = atoi(argv[1]);
    printf("process_count = %d\n", process_count);

    int required_fork_count = process_count;
SPAWN:

    for (int i = 0; i < required_fork_count; i++) {
        pid_t pid;
        switch (pid = fork()) {
            case -1:
                err(1, "fork");
                break;
            case 0:
                while (1) {
                    printf("child running %ld\n", (long)getpid());
                    sleep(1);
                }
                break;
            default:
                printf("spawned %ld\n", (long)pid);
                break; 
        }
        // 各子プロセスによるprintfが順番通りに並ぶように、わずかにsleepしておく
        millisleep(10);
    }

    while (1) {
        pid_t pid;
        int status;
        required_fork_count = 0;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            printf("child %ld died\n", (long)pid);
            required_fork_count += 1;
        }
        if (required_fork_count > 0) {
            goto SPAWN;
        }
        sleep(1);
    }

    return 0;
}
