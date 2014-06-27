#ifndef FS_INTERFACE_H
#define FS_INTERFACE_H

struct FileEntry;

int init_fs();
int create(char filename[]);
int removeFile(char filename[]);
int openFile(char filename[]);
int closeFile(int oftIndex);

int read(int oftIndex, char* buffer, int count); // return read bytes qty
int write(int oftIndex, char* buffer, int count);
int lseek(int oftIndex, int pos);

int list(struct FileEntry **result); // client gets ownership of pointer

#endif // FS_INTERFACE_H
