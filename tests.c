#include "tests.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log_wrapper.h"
#include "structs.h"


void simpleTest() {
    if (init_fs_log() >= 0) {
        create_log("aaaaaaaaaaaa");
        create_log("bbbbbbbbbbbb");
        create_log("cccccccccccc");
        create_log("filefilefile");
        removeFile_log("aaaaaaaaaaaa");

        int opened = openFile_log("bbbbbbbbbbbb");
        if (opened >= 0) {
            write_log(opened, "aaa aaa aaa", 11);
            closeFile_log(opened);
        }

        opened = openFile_log("bbbbbbbbbbbb");
        if (opened >= 0) {
            char buffer[11];
            memset(buffer, 0, 11);
            read_log(opened, buffer, 19);
        }

        opened = openFile_log("aaaaaaaaaaaa");
        if (opened >= 0) {
            write_log(opened, "qwertyuiopasdfghjklz", 20);
            lseek_log(opened, 15);
            char buffer[4];
            read_log(opened, buffer, 4);
            closeFile_log(opened);
        }

        struct FileEntry* fileList = 0;
        int files = list_log(&fileList);
        if (files > 0) {
            for (int i = 0; i < files; ++i) {
                printf("... FD  : %d\n", fileList[i].descriptor);
                printf("... LEN : %d\n", fileList[i].length);
                printf("... NAME: %s\n\n", fileList[i].name);
            }
        }

        free(fileList);
    }
}
