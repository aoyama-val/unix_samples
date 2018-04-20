//=============================================================================
// alarm()の使い方のサンプル
//  1秒間に何回fork()を呼べるか数える
//=============================================================================

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int fork_count = 0;
int sleep_seconds = 1;

void sighandler(int signo, siginfo_t *si, void *_context)
{
    printf("fork count = %d in %d seconds\n", fork_count, sleep_seconds);
    exit(0);
}

int main(int argc, char *argv[])
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sighandler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa, NULL);

    alarm(sleep_seconds);

    while (1) {
        pid_t child = fork();
        if (child == 0) {
            _exit(0);
        } else {
            fork_count++;
        }
    }

    return 0;
}
