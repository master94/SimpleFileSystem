#ifndef BASEMENT_H
#define BASEMENT_H

#include "structs.h"

// data blocks
int findFreeBlockIndex();
int markBlockUsed(int index);
int markBlockFree(int index);

// file descriptor magics
int findFreeFileDescriptorIndex();
int writeFileDescriptor(FD* pfd, int index);
int readFileDescriptor(FD* pfd, int index);
int removeFileDescriptor(int index);

// dir entries
int dirEntryCount();
int findFreeDirectoryEntryIndex();
int getFreeDirectoryEntryIndex(FD* fd); // when read new entry it must be in block bounds
int readDirectoryEntry(DE* pde, int index);
int writeDirectoryEntry(DE* pde, int index);

// OFT buffer
int writeBufferToDisk(OFT* oftItem, FD* fd);
int readBufferFromDisk(OFT* oftItem, FD* fd);

#endif
