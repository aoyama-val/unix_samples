//=============================================================================
//  sendfile()を使ってファイルをコピーする
//=============================================================================
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int
main(int argc, char *argv[])
{
    struct  stat    tmp;
    int     fdr, fdw;

    fdr = open("hoge1", O_RDONLY);
    fdw = open("hoge2", O_CREAT | O_WRONLY, 0644);

    if (!fdr) {
        printf("failed to open fdr\n");
        exit(1);
    }

    if (!fdw) {
        printf("failed to open fdw\n");
        exit(1);
    }

    fstat(fdr, &tmp);
    sendfile(fdw, fdr, NULL, tmp.st_size);

    close(fdr);
    close(fdw);
}
