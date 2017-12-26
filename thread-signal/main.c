// シグナルハンドラと組み合わせたサンプル

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

int a = 0;

void *thread_func(void *data) {
    printf("thread start. data = %d\n", *(int*)data);

    // 自身のスレッドIDを取得
    pthread_t self = pthread_self();
    printf("worker thread ID = %lu\n", self);

    a = 1;

    int n = strcmp("hoge", NULL);   // SEGVが起きる
    printf("strcmp = %d\n", n);

    return NULL;
}

void sigsegv_handler(int n)
{
    char * s = "caught SIGSEGV!\n";
    write(1, s, strlen(s));
    pthread_exit(NULL);
}

void set_sighandler()
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigsegv_handler;

    if (sigaction(SIGSEGV, &sa, NULL) != 0) {
        perror("sigaction");
    }
}

int main(void)
{
    pthread_t worker;

    set_sighandler();

    printf("main start. a = %d\n", a);

    int thread_arg = 42;    // thread_func()に渡すデータ

    // スレッド開始
    if (pthread_create(&worker, NULL, thread_func, &thread_arg) != 0) {
        perror("pthread_create");
        exit(1);
    }

    printf("worker thread ID = %lu\n", worker);

    pthread_t self = pthread_self();
    printf("main thread ID = %lu\n", self);

    pthread_join(worker, NULL); // スレッドが終わるまで待つ
    printf("a = %d\n", a);

    return 0;
}
