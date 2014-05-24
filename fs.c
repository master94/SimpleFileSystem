#include "fs_interface.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fake_driver.h"
#include "status.h"
#include "defines.h"
#include "basement.h"


int findDirEntryByFileName(char name[FILE_NAME_SIZE]);

OFT* getFileTable() {
    static OFT* table = 0;
    static int initialized = 0;

    if (!initialized) {
        table = malloc(FILE_TABLE_SIZE * sizeof(OFT));

        for (int i = 0; i < FILE_TABLE_SIZE; ++i) {
            table[i].fileDescriptor = -1;
            table[i].currPos = -1;
            memset(table[i].buffer, 0, BLOCK_SIZE);
        }

        initialized = 1;
    }

    return table;
}

int getFreeOftItemIndex() {
    OFT* table = getFileTable();

    for (int i = 0; i < FILE_TABLE_SIZE; ++i) {
        if (table[i].fileDescriptor == -1)
            return i;
    }

    return -1;
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

int openFile(char filename[FILE_NAME_SIZE]) {
    const int dirEntry = findDirEntryByFileName(filename);
    if (dirEntry < 0)
        return dirEntry;

    DE de;
    int status = readDirectoryEntry(&de, dirEntry);
    if (status < 0)
        return status;

    int freeOftItemIndex = getFreeOftItemIndex();
    if (freeOftItemIndex < 0)
        return freeOftItemIndex;

    OFT* oftItem = &getFileTable()[freeOftItemIndex];
    oftItem->currPos = 0;
    oftItem->fileDescriptor = de.fileDescriptor;

    FD fd;
    status = readFileDescriptor(&fd, de.fileDescriptor);
    if (fd.length > 0) {
        const int blockIndex = fd.blocks[0];
        if (blockIndex > -1) {
            status = read_block(SERVICE_BLOCKS + blockIndex, oftItem->buffer);
            if (status < 0)
                return status;
        }
    }

    return freeOftItemIndex;
}

int closeFile(int oftIndex) {
    OFT* oftItem = &getFileTable()[oftIndex];
    if (oftItem->currPos < 0)
        return -1;

    FD fd;
    int status = readFileDescriptor(&fd, oftItem->fileDescriptor);
    if (status < 0)
        return status;

    if (oftItem->currPos > fd.length)
        fd.length = oftItem->currPos;

    status = writeBufferToDisk(oftItem, &fd);
    if (status < 0)
        return status;

    status = writeFileDescriptor(&fd, oftItem->fileDescriptor);
    if (status < 0)
        return status;

    oftItem->fileDescriptor = -1;
    oftItem->currPos = 0;

    return 0;
}

int read(int oftIndex, char* buffer, int count) {
    OFT* oftItem = &getFileTable()[oftIndex];
    if (oftItem->fileDescriptor == -1)
        return -1;

    FD fd;
    int status = readFileDescriptor(&fd, oftItem->fileDescriptor);
    if (status < 0)
        return status;

    int read = 0;
    char* pos = buffer;

    while (count - read > 0) {
        if (fd.length - oftItem->currPos == 0)
            break;

        const int offset = oftItem->currPos % BLOCK_SIZE;
        const int qty = min(count - read, min(BLOCK_SIZE - offset, fd.length - oftItem->currPos));
        memcpy(pos, oftItem->buffer + offset, qty);
        pos += qty;
        read += qty;
        oftItem->currPos += qty;

        if (offset + qty == BLOCK_SIZE) {
            status = writeBufferToDisk(oftItem, &fd);
            if (status < 0)
                return status;

            status = readBufferFromDisk(oftItem, &fd);
            if (status < 0)
                return status;
        }
    }

    return read;
}

int write(int oftIndex, char* buffer, int count) {
    OFT* oftItem = &getFileTable()[oftIndex];
    if (oftItem->fileDescriptor == -1)
        return -1;

    FD fd;
    int status = readFileDescriptor(&fd, oftItem->fileDescriptor);
    if (status < 0)
        return status;

    int written = 0;
    char* pos = buffer;

    while (count - written > 0) {
        if (fd.length - oftItem->currPos == 0)
            break;

        const int offset = oftItem->currPos % BLOCK_SIZE;
        const int qty = min(count - written, BLOCK_SIZE - offset);
        memcpy(oftItem->buffer + offset, pos, qty);
        pos += qty;
        written += qty;
        oftItem->currPos += qty;
        fd.length = fd.length > oftItem->currPos ? fd.length : oftItem->currPos;

        if (offset + qty == BLOCK_SIZE) {
            status = writeBufferToDisk(oftItem, &fd);
            if (status < 0)
                return status;

            const int innerBlockIndex = (oftItem->currPos - 1) / BLOCK_SIZE;
            if (innerBlockIndex == BLOCK_PER_FILE)
                return -1;

            int blockIndex = fd.blocks[innerBlockIndex];
            if (blockIndex == -1) {
                blockIndex = findFreeBlockIndex();
                if (blockIndex < 0)
                    return blockIndex;

                status = markBlockUsed(blockIndex);
                if (status < 0)
                    return status;

                fd.blocks[innerBlockIndex] = blockIndex;
            }

            status = readBufferFromDisk(oftItem, &fd);
            if (status < 0)
                return status;
        }
    }

    status = writeFileDescriptor(&fd, oftItem->fileDescriptor);
    if (status < 0)
        return status;

    return written;
}

/*
 *
 *
 *
 *
 */

