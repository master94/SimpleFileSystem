#include <stdio.h>
#include "fs_interface.h"

int main() {
    init_fs();
    printf("STAT %d\n", create("aaaaaaaaaaaa"));
    //printf("STAT %d\n", create("bbbbbbbbbbbb"));
    //printf("STAT %d\n", create("cccccccccccc"));
    //printf("STAT %d\n", create("filefilefile"));
    printf("STAT %d\n", removeFile("bbbbbbbbbbbb"));
    dump_disk();
    return 0;
}

