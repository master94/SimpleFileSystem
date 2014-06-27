#include "log_wrapper.h"

#include <stdio.h>

#include "fs_interface.h"
#include "statuses.h"


const char* getStatusText(int status) {
    if (status >= 0)
        return "Success";

    switch(status) {
    case NO_FREE_BLOCK:
        return "No free blcok found";
    case NO_FREE_FILE_DESCRIPTOR:
        return "No free file descriptor found";
    case READ_BLOCK_ERROR:
        return "Block read error";
    case WRITE_BLOCK_ERROR:
        return "Block write error";
    case NO_FREE_DIR_ENTRY:
        return "No free dir entry found";
    case NO_DIR_ENTRY_FOUND:
        return "No dir entry found";
    case NO_FREE_OFT_ITEM_FOUND:
        return "No free OFT itrem found";
    case FILE_NOT_OPENED:
        return "File not opened";
    case BAD_OFT_INDEX:
        return "Bad OFT index";
    default:
        return "No such return status";
    }
}

int init_fs_log() {
    const int status = init_fs();
    printf("INIT FS: %s\n", getStatusText(status));
    return status;
}

int create_log(char filename[]) {
    const int status = create(filename);
    printf("CREATE FILE \"%s\": %s\n", filename, getStatusText(status));
    return status;
}

int removeFile_log(char filename[]) {
    const int status = removeFile(filename);
    printf("REMOVE FILE \"%s\": %s\n", filename, getStatusText(status));
    return status;
}

int openFile_log(char filename[]) {
    const int status = openFile(filename);
    printf("OPEN FILE \"%s\": %s\n", filename, getStatusText(status));
    if (status >= 0)
        printf("OFT INDEX: %d\n", status);
    return status;
}

int closeFile_log(int oftIndex) {
    const int status = closeFile(oftIndex);
    printf("CLOSE FILE %d: %s\n", oftIndex, getStatusText(status));
    return status;
}

int read_log(int oftIndex, char* buffer, int count) {
    const int status = read(oftIndex, buffer, count);
    printf("READ FILE %d: %s\n", oftIndex, getStatusText(status));
    if (status >= 0)
        printf("READ DATA: \"%.*s\"\n", count, buffer);
    return status;
}

int write_log(int oftIndex, char* buffer, int count) {
    const int status = write(oftIndex, buffer, count);
    printf("WRITE FILE %d: %s\n", oftIndex, getStatusText(status));
    if (status >= 0)
        printf("WRITTEN DATA: \"%.*s\"\n", count, buffer);
    return status;
}

int lseek_log(int oftIndex, int pos) {
    const int status = lseek(oftIndex, pos);
    printf("SEEK FILE %d: %s\n", oftIndex, getStatusText(status));
    return status;
}

int list_log(struct FileEntry **result) {
    const int status = list(result);
    printf("LIST: ");
    status > 0 ? printf("%d FILES\n", status) : printf("%s\n", getStatusText(status));
    return status;
}
