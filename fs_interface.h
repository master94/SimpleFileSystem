#ifndef FS_INTERFACE_H
#define FS_INTERFACE_H

typedef struct {
    int descriptor;
    int length;
    char* name;
} FileEntry;


int init_fs();
int create(char filename[]);
int removeFile(char filename[]);
int openFile(char filename[]);
int closeFile(int oftIndex);

int read(int oftIndex, char* buffer, int count); // return read bytes qty
int write(int oftIndex, char* buffer, int count);
int lseek(int oftIndex, int pos);

int list(FileEntry **result); // client gets ownership of pointer

void dump_disk();
void dump_oft();

/*void destroy(const char* filename);
void open(const char* filename);
void close(int index);
void read(int index, char* mem_area, int count);
void write(int index, char* mem_area, int count);
void lseek(int index, int pos);
struct FileEntry* directory();*/

#endif // FS_INTERFACE_H
