#ifndef BASEMENT_H
#define BASEMENT_H

#include "structs.h"

// data blocks
int findFreeBlockIndex();
int markBlockUsed(int index);
int markBlockFree(int index);

// file descriptor magics
int findFreeFileDescriptorIndex();
int writeFileDescriptor(FileDescriptor* pfd, int index);
int readFileDescriptor(FileDescriptor* pfd, int index);
int removeFileDescriptor(int index);

// dir entries
int dirEntryCount();
int findFreeDirectoryEntryIndex();
int getFreeDirectoryEntryIndex(FileDescriptor* fd); // when read new entry it must be in block bounds
int readDirectoryEntry(DirectoryEntry* pde, int index);
int writeDirectoryEntry(DirectoryEntry* pde, int index);

// OpenFileTable buffer
int writeBufferToDisk(OpenFileTable* oftItem, FileDescriptor* fd);
int readBufferFromDisk(OpenFileTable* oftItem, FileDescriptor* fd);

#endif
