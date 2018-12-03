//=============================================================================
//   mmapによる共有メモリのサンプル
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
    char*p = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    printf("%p\n",p );

    if (fork() == 0) {
        strcpy(p, "From child");
        exit(0);
    }

    sleep(1);

    printf("Parent: %s\n", p);

    return 0;
}
