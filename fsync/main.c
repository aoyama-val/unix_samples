//=============================================================================
//   fsyncの使い方のサンプル
//=============================================================================
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    // 1秒に1回fsyncを10回行う
    int fd = open("test.out", O_CREAT | O_WRONLY, 0644);
    for (int i = 0; i < 10; i++) {
        write(fd, "hoge\n", 5);
        fsync(fd);
        sleep(1);
    }
    close(fd);
    return 0;
}
