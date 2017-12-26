// pthreadのHello, world!

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int a = 0;

void *thread_func(void *data) {
    printf("thread start. data = %d\n", *(int*)data);

    // 自身のスレッドIDを取得
    pthread_t self = pthread_self();
    printf("worker thread ID = %lu\n", self);

    a = 1;

    return NULL;
}

int main(void)
{
    pthread_t worker;

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
