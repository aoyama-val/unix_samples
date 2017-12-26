#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

jmp_buf jbuf;

void hoge()
{
    printf("hoge\n");
    longjmp(jbuf, 123);
    printf("never reach here\n");
}

int main(int argc, char *argv[])
{
    int n;

    printf("main\n");

    if ((n = setjmp(jbuf)) == 0) {
        printf("setjmp success\n");
        hoge();
    } else {
        printf("n = %d\n", n);
    }

    return 0;
}
