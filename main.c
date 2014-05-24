#include <stdio.h>
#include <string.h>

#include "fs_interface.h"

int main() {
    init_fs();
    printf("STAT %d\n", create("aaaaaaaaaaaa"));
    printf("STAT %d\n", create("bbbbbbbbbbbb"));
    //printf("STAT %d\n", create("cccccccccccc"));
    //printf("STAT %d\n", create("filefilefile"));
    //printf("STAT %d\n", removeFile("aaaaaaaaaaaa"));

    //dump_oft();

    int opened = openFile("bbbbbbbbbbbb");
    printf("OPEN %d\n", opened);

    printf("WRITE %d\n", write(opened, "aaa aaa aaa", 11));
    closeFile(opened);
    opened = openFile("bbbbbbbbbbbb");
    char buffer[11];
    memset(buffer, 0, 11);

    printf("READ %d\n", read(opened, buffer, 19));
    printf("%s\n", buffer);

    opened = openFile("aaaaaaaaaaaa");
    printf("OPEN %d\n", opened);
    printf("WRITE %d\n", write(opened, "123 123 123 123 1234", 20));
    closeFile(opened);

    dump_oft();

    dump_disk();
    return 0;
}

