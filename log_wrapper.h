#ifndef LOG_WRAPPER_H
#define LOG_WRAPPER_H

struct FileEntry;

int init_fs_log();
int create_log(char filename[]);
int removeFile_log(char filename[]);
int openFile_log(char filename[]);
int closeFile_log(int oftIndex);

int read_log(int oftIndex, char* buffer, int count);
int write_log(int oftIndex, char* buffer, int count);
int lseek_log(int oftIndex, int pos);

int list_log(struct FileEntry **result);

#endif
