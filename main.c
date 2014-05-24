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
    printf("WRITE %d\n", write(opened, "qwertyuiopasdfghjklz", 20));
    printf("SEEK %d\n", lseek(opened, 15));
    printf("READ %d\n", read(opened, buffer, 4));
    for (int i = 0; i < 4; ++i)
        printf("%c", buffer[i]);
    printf("\n");
    closeFile(opened);

    FileEntry* fileList = 0;
    int files = list(&fileList);
    printf("FILES %d\n", files);

    for (int i = 0; i < files; ++i) {
        printf("... FD  : %d\n", fileList[i].descriptor);
        printf("... LEN : %d\n", fileList[i].length);
        printf("... NAME: %s\n\n", fileList[i].name);
    }


    dump_oft();

    dump_disk();
    return 0;
}

