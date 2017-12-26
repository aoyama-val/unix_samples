//=============================================================================
//  SEGVをハンドルし、何事もなかったかのように実行を続けるサンプル
//  https://stackoverflow.com/questions/8456085/why-cant-i-ignore-sigsegv-signal
//=============================================================================

#define _GNU_SOURCE

#include <ucontext.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

int *p = NULL;
int n = 123;

void sighandler(int signo, siginfo_t *si, void *_context)
{
    ucontext_t* context = (ucontext_t*)_context;
    printf("Handler executed for signal %d\n", signo);  // printfはリエントラントではないので本来は呼んではいけない。
    context->uc_mcontext.gregs[REG_RAX] = (long)&n;   // レジスタRAXに123を代入
}

int main(int argc, char *argv[])
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sighandler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);
    printf("%d\n", *p); // *pによりSEGV発生

    return 0;
}
