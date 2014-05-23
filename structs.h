#ifndef STRUCTS_H
#define STRUCTS_H

#include "defines.h"


typedef struct FileDescriptor {
    int length;
    int blocks[BLOCK_PER_FILE];
} FD;

typedef struct DirectoryEntry {
    int fileDescriptor;
    char filename[FILE_NAME_SIZE];
} DE;

typedef struct OpenFileTable {
    int fileDescriptor;
    int currPos;
    char buffer[BLOCK_SIZE];
} OFT;

#endif
