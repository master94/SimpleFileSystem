#include "basement.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "fake_driver.h"
#include "defines.h"
#include "errors.h"

void bytesToInt(const char* bytes, int* value);
void intToBytes(char* bytes, const int* value);

int readDataWithOffset(int address, char* data, int size);
int writeDataWithOffset(int address, const char* data, int size);


int dirEntryCount() {
    return BLOCK_PER_FILE * BLOCK_SIZE / sizeof(DE);
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

int findFreeFileDescriptorIndex() {
    const int offset = ceil((double)(BLOCKS - SERVICE_BLOCKS) / (BIT_PER_BYTE * BLOCK_SIZE)) * BLOCK_SIZE;
    int index = 0;

    char buffer[sizeof(int)];
    int value = 0;

    while (offset + index * sizeof(FD) < SERVICE_BLOCKS * BLOCK_SIZE) {
        int status = readDataWithOffset(offset + index * sizeof(FD), buffer, sizeof(int));
        if (status < 0)
            return status;

        bytesToInt(buffer, &value);
        if (value == -1)
            return index;
        ++index;
    }

    return NO_FREE_FILE_DESCRIPTOR;
}

int readDataWithOffset(int address, char* data, int size) {
    int block = address / BLOCK_SIZE;
    int offset = address - block * BLOCK_SIZE;

    char buffer[BLOCK_SIZE];
    int copied = 0;
    while (size - copied > 0) {
        int toCopy = min(size - copied, BLOCK_SIZE - offset);

        int status = read_block(block, buffer);
        if (status < 0)
            return READ_BLOCK_ERROR;

        memcpy(data + copied, buffer + offset, toCopy);

        ++block;
        copied += toCopy;
        offset = 0;
    }

    return 0;
}

int writeDataWithOffset(int address, const char* data, int size) {
    int block = address / BLOCK_SIZE;
    int offset = address - block * BLOCK_SIZE;

    char buffer[BLOCK_SIZE];
    int copied = 0;
    while (size - copied > 0) {
        int toCopy = min(size - copied, BLOCK_SIZE - offset);

        int status = read_block(block, buffer);
        if (status < 0)
            return status;

        memcpy(buffer + offset, data + copied, toCopy);

        status = write_block(block, buffer);
        if (status < 0)
            return WRITE_BLOCK_ERROR;

        ++block;
        copied += toCopy;
        offset = 0;
    }

    return 0;
}

int writeFileDescriptor(FD* pfd, int index) {
    const int offset = fileDescrStartBlock * BLOCK_SIZE + index * sizeof(FD);
    return writeDataWithOffset(offset, (char*)pfd, sizeof(FD));
}

int readFileDescriptor(FD* pfd, int index) {
    const int offset = fileDescrStartBlock * BLOCK_SIZE + index * sizeof(FD);
    return readDataWithOffset(offset, (char*)pfd, sizeof(FD));
}

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

    int status = readDataWithOffset(0, buffer, mapBytes);
    if (status < 0)
        return status;

    for (int i = 0; i < dataBlocks; ++i) {
        const int byteIndex = i / BIT_PER_BYTE;
        const int bitIndex = i - byteIndex * BIT_PER_BYTE;

        const unsigned char marker = 1 << (BIT_PER_BYTE - 1 - bitIndex);
        if (!(buffer[byteIndex] & marker))
            return i;
    }

    return NO_FREE_BLOCK;
}

int markBlockUsed(int index) {
    const int byteIndex = index / BIT_PER_BYTE;
    const int bitIndex = index - byteIndex * BIT_PER_BYTE;
    const unsigned char marker = 1 << (BIT_PER_BYTE - 1 - bitIndex);

    char byte;
    int status = readDataWithOffset(byteIndex, &byte, 1);
    if (status < 0)
        return status;

    byte |= marker;

    status = writeDataWithOffset(byteIndex, &byte, 1);
    if (status < 0)
        return status;

    return 0;
}

int markBlockFree(int index) {
    const int byteIndex = index / BIT_PER_BYTE;
    const int bitIndex = index - byteIndex * BIT_PER_BYTE;
    const unsigned char marker = ~(1 << (BIT_PER_BYTE - 1 - bitIndex));

    char byte;
    int status = readDataWithOffset(byteIndex, &byte, 1);
    if (status < 0)
        return status;

    byte &= marker;

    status = writeDataWithOffset(byteIndex, &byte, 1);
    if (status < 0)
        return status;

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

        const int address = (SERVICE_BLOCKS + fd->blocks[block]) * BLOCK_SIZE + offset % BLOCK_SIZE;

        int status = readDataWithOffset(address, (char*)&entry, sizeof(DE));
        if (status < 0)
            return status;

        if (entry.fileDescriptor == -1)
            return offset / sizeof(DE);

        offset += sizeof(DE);
    }

    return NO_FREE_DIR_ENTRY;
}

int readDirectoryEntry(DE* pde, int index) {
    if ((index + 1) * sizeof(DE) > BLOCK_PER_FILE * BLOCK_SIZE)
        return -1;

    FD d;
    int status = readFileDescriptor(&d, 0); //get dir desr
    if (status < 0)
        return status;

    const int innerBlockIndex = index * sizeof(DE) / BLOCK_SIZE;
    const int blockIndex = SERVICE_BLOCKS + d.blocks[innerBlockIndex];
    const int offset = index * sizeof(DE) % BLOCK_SIZE;

    status = readDataWithOffset(blockIndex * BLOCK_SIZE + offset, (char*)pde, sizeof(DE));
    if (status < 0)
        return status;

    return 0;
}

int writeDirectoryEntry(DE* pde, int index) {
    if ((index + 1) * sizeof(DE) > BLOCK_PER_FILE * BLOCK_SIZE)
        return -1;

    FD d;
    int status = readFileDescriptor(&d, 0); //get dir descr
    if (status < 0)
        return status;

    const int innerBlockIndex = index * sizeof(DE) / BLOCK_SIZE;
    const int blockIndex = SERVICE_BLOCKS + d.blocks[innerBlockIndex];
    const int offset = index * sizeof(DE) % BLOCK_SIZE;

    status = writeDataWithOffset(blockIndex * BLOCK_SIZE + offset, (char*)pde, sizeof(DE));
    if (status < 0)
        return status;

    return 0;
}

int findFreeDirectoryEntryIndex() {
    FD d;
    int status = readFileDescriptor(&d, 0); //get dir desr
    if (status < 0)
        return status;

    if (d.length == 0 && d.blocks[0] == -1) {
        for (int i = 0; i < BLOCK_PER_FILE; ++i) {
            const int freeBlockIndex = findFreeBlockIndex();
            status = markBlockUsed(freeBlockIndex);
            if (status < 0)
                return status;
            d.blocks[i] = freeBlockIndex;
        }

        DE de;
        de.fileDescriptor = -1;
        memset(de.filename, 0, FILE_NAME_SIZE);

        status = writeFileDescriptor(&d, 0);
        if (status < 0)
            return status;

        for (int i = 0; i < dirEntryCount(); ++i) {
            status = writeDirectoryEntry(&de, i);
            if (status < 0)
                return status;
        }

        return 0;
    }
    else {
        return getFreeDirectoryEntryIndex(&d);
    }
}

int writeBufferToDisk(OFT* oftItem, FD* fd) {
    const int innerBlockIndex = (oftItem->currPos - 1) / BLOCK_SIZE;
    if (innerBlockIndex >= BLOCK_PER_FILE)
        return -1;

    int status = 0;

    int blockIndex = fd->blocks[innerBlockIndex];
    if (blockIndex == -1) {
        blockIndex = findFreeBlockIndex();
        if (blockIndex < 0)
            return blockIndex;

        status = markBlockUsed(blockIndex);
        if (status < 0)
            return status;

        fd->blocks[innerBlockIndex] = blockIndex;
    }

    status = write_block(SERVICE_BLOCKS + blockIndex, oftItem->buffer);
    if (status < 0)
        return status;

    status = writeFileDescriptor(fd, oftItem->fileDescriptor);
    if (status < 0)
        return status;

    return 0;
}

int readBufferFromDisk(OFT* oftItem, FD* fd) {
    const int innerBlockIndex = oftItem->currPos / BLOCK_SIZE;
    if (innerBlockIndex >= BLOCK_PER_FILE)
        return -1;

    const int blockIndex = fd->blocks[innerBlockIndex];
    int status = read_block(SERVICE_BLOCKS + blockIndex, oftItem->buffer);
    if (status < 0)
        return status;

    return 0;
}
