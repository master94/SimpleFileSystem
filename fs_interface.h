#ifndef FS_INTERFACE_H
#define FS_INTERFACE_H


struct FileEntry {
    const char* name;
    int length;
};

int init_fs();
int create(char filename[]);
int removeFile(char filename[]);

void writeDataWithOffset(int address, const char* data, int size);
int findFreeDirectoryEntryIndex();

void dump_disk();
/*void destroy(const char* filename);
void open(const char* filename);
void close(int index);
void read(int index, char* mem_area, int count);
void write(int index, char* mem_area, int count);
void lseek(int index, int pos);
struct FileEntry* directory();*/

#endif // FS_INTERFACE_H
