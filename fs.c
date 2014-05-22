#include "fs_interface.h"
#include "status.h"
#include "fake_driver.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>


#define SERVICE_BLOCKS 28
#define BLOCK_PER_FILE 3

#define CYLINDERS 1
#define SURFACES 2
#define SECTORS 78
#define BLOCK_SIZE 16
#define BLOCKS (CYLINDERS * SURFACES * SECTORS)

#define DESC_SIZE (4 * sizeof(int))

#define BIT_PER_BYTE 8

#define min(a, b) (a > b ? b : a)

#define fileDescrStartBlock (ceil((double)BLOCK_SIZE / (BLOCKS - SERVICE_BLOCKS)))
#define fileDescrEndBlock (SERVICE_BLOCKS - 1)

#define FILE_NAME_SIZE 12


typedef struct FileDescriptor {
    int length;
    int blocks[BLOCK_PER_FILE];
} FD;

typedef struct DirectoryEntry {
    int fileDescriptor;
    char filename[FILE_NAME_SIZE];
} DE;


void readDataWithOffset(int address, char* data, int size);
int readFileDescriptor(FD* pfd, int index);
int writeFileDescriptor(FD *pfd, int index);
int dirEntryCount();



void bytesToInt(const char* bytes, int* value) {
    memcpy((char*)value, bytes, sizeof(int));
}

void intToBytes(char* bytes, const int* value) {
    memcpy(bytes, (char*)value, sizeof(int));
}

int findFreeBlockIndex() {
    const int dataBlocks = BLOCKS - SERVICE_BLOCKS;
    const int mapBytes = ceil((double)dataBlocks / BIT_PER_BYTE);
    char buffer[mapBytes];
    readDataWithOffset(0, buffer, mapBytes);

    for (int i = 0; i < dataBlocks; ++i) {
        const int byteIndex = i / BIT_PER_BYTE;
        const int bitIndex = i - byteIndex * BIT_PER_BYTE;

        const unsigned char marker = 1 << (BIT_PER_BYTE - 1 - bitIndex);
        if (!(buffer[byteIndex] & marker))
            return i;
    }

    return -1;
}

int markBlockUsed(int index) {
    int byteIndex = index / BIT_PER_BYTE;
    const int bitIndex = index - byteIndex * BIT_PER_BYTE;
    const unsigned char marker = 1 << (BIT_PER_BYTE - 1 - bitIndex);

    char buffer[1];
    readDataWithOffset(byteIndex, buffer, 1);
    buffer[0] |= marker;
    writeDataWithOffset(byteIndex, buffer, 1);

    return 0;
}

int markBlockFree(int index) {
    int byteIndex = index / BIT_PER_BYTE;
    const int bitIndex = index - byteIndex * BIT_PER_BYTE;
    const unsigned char marker = ~(1 << (BIT_PER_BYTE - 1 - bitIndex));

    char buffer[1];
    readDataWithOffset(byteIndex, buffer, 1);
    buffer[0] &= marker;
    writeDataWithOffset(byteIndex, buffer, 1);

    return 0;
}

// when read new entry it must be in block bounds
int getFreeDirectoryEntryIndex(FD* fd) {
    DE entry;
    int offset = 0;

    while (1) {
        const int block = offset / BLOCK_SIZE;
        if (block >= BLOCK_PER_FILE || offset + sizeof(DE) > BLOCK_PER_FILE * BLOCK_SIZE)
            break;

        if (fd->blocks[block] == -1)
            offset = (block + 1) * BLOCK_SIZE;

        const int address = (SERVICE_BLOCKS + fd->blocks[block]) * BLOCK_SIZE;

        readDataWithOffset(address, (char*)&entry, sizeof(DE));

        if (entry.fileDescriptor == -1)
            return offset / sizeof(DE);

        offset += sizeof(DE);
    }

    return -1;
}

inline int dirEntryCount() {
    return BLOCK_PER_FILE * BLOCK_SIZE / sizeof(DE);
}

int readDirectoryEntry(DE* pde, int index) {
    if ((index + 1) * sizeof(DE) > BLOCK_PER_FILE * BLOCK_SIZE)
        return -1;

    FD d;
    readFileDescriptor(&d, 0); //get dir desr

    const int innerBlockIndex = index * sizeof(DE) / BLOCK_SIZE;
    const int blockIndex = SERVICE_BLOCKS + d.blocks[innerBlockIndex];
    const int offset = index * sizeof(DE) % BLOCK_SIZE;

    readDataWithOffset(blockIndex * BLOCK_SIZE + offset, (char*)pde, sizeof(DE));

    return 0;
}

int writeDirectoryEntry(DE* pde, int index) {
    if ((index + 1) * sizeof(DE) > BLOCK_PER_FILE * BLOCK_SIZE)
        return -1;

    FD d;
    readFileDescriptor(&d, 0); //get dir desr

    const int innerBlockIndex = index * sizeof(DE) / BLOCK_SIZE;
    const int blockIndex = SERVICE_BLOCKS + d.blocks[innerBlockIndex];
    const int offset = index * sizeof(DE) % BLOCK_SIZE;

    writeDataWithOffset(blockIndex * BLOCK_SIZE + offset, (char*)pde, sizeof(DE));

    return 0;
}

int findFreeDirectoryEntryIndex() {
    FD d;
    readFileDescriptor(&d, 0); //get dir desr

    if (d.length == 0 && d.blocks[0] == -1) {
        for (int i = 0; i < BLOCK_PER_FILE; ++i) {
            const int freeBlockIndex = findFreeBlockIndex();
            markBlockUsed(freeBlockIndex);
            d.blocks[i] = freeBlockIndex;
        }

        DE de;
        de.fileDescriptor = -1;
        memset(de.filename, 0, FILE_NAME_SIZE);

        writeFileDescriptor(&d, 0);

        for (int i = 0; i < dirEntryCount(); ++i)
            writeDirectoryEntry(&de, i);

        return 0;
    }
    else {
        return getFreeDirectoryEntryIndex(&d);
    }
}

int findFreeFileDescriptorIndex() {
    const int offset = ceil((double)(BLOCKS - SERVICE_BLOCKS) / (BIT_PER_BYTE * BLOCK_SIZE)) * BLOCK_SIZE;
    int index = 0;

    char buffer[sizeof(int)];
    int value = 0;

    while (offset + index * BLOCK_SIZE < SERVICE_BLOCKS * BLOCK_SIZE) {
        readDataWithOffset(offset + index * BLOCK_SIZE, buffer, sizeof(int));
        bytesToInt(buffer, &value);
        if (value == -1)
            return index;
        ++index;
    }

    return -1;
}

void readDataWithOffset(int address, char* data, int size) {
    int block = address / BLOCK_SIZE;
    int offset = address - block * BLOCK_SIZE;

    char buffer[BLOCK_SIZE];
    int copied = 0;
    while (size - copied > 0) {
        int toCopy = min(size - copied, BLOCK_SIZE - offset);

        read_block(block, buffer);
        memcpy(data + copied, buffer + offset, toCopy);

        ++block;
        copied += toCopy;
        offset = 0;
    }
}

void writeDataWithOffset(int address, const char* data, int size) {
    int block = address / BLOCK_SIZE;
    int offset = address - block * BLOCK_SIZE;

    char buffer[BLOCK_SIZE];
    int copied = 0;
    while (size - copied > 0) {
        int toCopy = min(size - copied, BLOCK_SIZE - offset);

        read_block(block, buffer);
        memcpy(buffer + offset, data + copied, toCopy);
        write_block(block, buffer);

        ++block;
        copied += toCopy;
        offset = 0;
    }
}

void writeToBlock(char* block, char* data, int offset, int size) {
    for (int i = 0; i < size; ++i) {
        block[offset + i] = data[i];
    }
}

int writeFileDescriptor(FD* pfd, int index) {
    const int offset = fileDescrStartBlock * BLOCK_SIZE + index * DESC_SIZE;
    writeDataWithOffset(offset, (char*)pfd, sizeof(FD));

    return 0;
}

int readFileDescriptor(FD* pfd, int index) {
    const int offset = fileDescrStartBlock * BLOCK_SIZE + index * DESC_SIZE;
    readDataWithOffset(offset, (char*)pfd, sizeof(FD));

    return 0;
}

int init_fs() {
    init(BLOCKS, BLOCK_SIZE); //FAKE

    char block_stub[BLOCK_SIZE];
    memset(block_stub, 0, BLOCK_SIZE);

    for (int i = 0; i < BLOCKS; ++i) {
        write_block(i, block_stub);
    }

    FD fd;
    fd.length = 0;
    for (int i = 0; i < BLOCK_PER_FILE; ++i)
        fd.blocks[i] = -1;

    const int fileDescriptorsQty = fileDescrEndBlock - fileDescrStartBlock + 1;

    writeFileDescriptor(&fd, 0); // dir
    for (int i = 1; i < fileDescriptorsQty; ++i) {
        fd.length = -1;
        writeFileDescriptor(&fd, i);
    }

    return 0;
}

int create(char filename[FILE_NAME_SIZE]) {
    const int freeDescr = findFreeFileDescriptorIndex();
    if (freeDescr < 0)
        return freeDescr;

    const int freeDirEntry = findFreeDirectoryEntryIndex();
    if (freeDirEntry < 0)
        return freeDirEntry;

    DE de;
    de.fileDescriptor = freeDescr;
    memcpy(de.filename, filename, FILE_NAME_SIZE);

    int status = writeDirectoryEntry(&de, freeDirEntry);
    if (status < 0)
        return status;

    FD fd;
    status = readFileDescriptor(&fd, freeDescr);
    if (status < 0)
        return status;

    fd.length = 0;

    status = writeFileDescriptor(&fd, freeDescr);
    if (status < 0)
        return status;

    return 0;
}

int removeFileDescriptor(int index) {
    FD fd;
    int status = readFileDescriptor(&fd, index);
    if (status < 0)
        return status;

    for (int i = 0; i < BLOCK_PER_FILE; ++i) {
        const int blockIndex = fd.blocks[i];
        if (blockIndex == -1)
            continue;

        status = markBlockFree(blockIndex);
        if (status < 0)
            return status;

        fd.blocks[i] = -1;
    }

    fd.length = -1;

    status = writeFileDescriptor(&fd, index);
    if (status < 0)
        return status;

    return 0;
}

int removeFile(char filename[FILE_NAME_SIZE]) {
    const int dirEntry = findDirEntryByFileName(filename);
    if (dirEntry < 0)
        return dirEntry;

    DE de;
    int status = readDirectoryEntry(&de, dirEntry);
    if (status < 0)
        return status;

    const int descriptor = de.fileDescriptor;

    de.fileDescriptor = -1;
    memset(de.filename, 0, FILE_NAME_SIZE);

    status = writeDirectoryEntry(&de, dirEntry);

    status = removeFileDescriptor(descriptor);
    if (status < 0)
        return status;

    return 0;
}

int findDirEntryByFileName(char name[FILE_NAME_SIZE]) {
    DE de;
    for (int i = 0; i < dirEntryCount(); ++i) {
        readDirectoryEntry(&de, i);
        if (de.fileDescriptor != -1) {
            int equal = 1;

            int pos = 0;
            while (pos < FILE_NAME_SIZE && name[pos] != '\0') {
                if (name[pos] != de.filename[pos]) {
                    equal = 0;
                    break;
                }
                ++pos;
            }

            if (equal)
                return i;
        }
    }

    return -1;
}

