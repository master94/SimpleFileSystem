#include "fs_interface.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include <driver_stub.h>

#include "status.h"
#include "defines.h"
#include "basement.h"
#include "errors.h"
#include "structs.h"


int findDirEntryByFileName(char name[FILE_NAME_SIZE]) {
    DirectoryEntry de;
    for (int i = 0; i < dirEntryCount(); ++i) {
        int status = readDirectoryEntry(&de, i);
        if (status < 0)
            return status;

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

    return NO_DIR_ENTRY_FOUND;
}

OpenFileTable* getFileTable() {
    static OpenFileTable* table = 0;
    static int initialized = 0;

    if (!initialized) {
        table = malloc(FILE_TABLE_SIZE * sizeof(OpenFileTable));

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
    OpenFileTable* table = getFileTable();

    for (int i = 0; i < FILE_TABLE_SIZE; ++i) {
        if (table[i].fileDescriptor == -1)
            return i;
    }

    return NO_FREE_OFT_ITEM_FOUND;
}

int init_fs() {
    int status = init(CYLINDERS, SURFACES, SECTORS, BLOCK_SIZE);
    if (status < 0)
        return status;

    char block_stub[BLOCK_SIZE];
    memset(block_stub, 0, BLOCK_SIZE);

    for (int i = 0; i < BLOCKS; ++i) {
        status = write_block(i, block_stub);
        if (status < 0)
            return status;
    }

    FileDescriptor fd;
    fd.length = 0;
    for (int i = 0; i < BLOCK_PER_FILE; ++i)
        fd.blocks[i] = -1;

    const int fileDescriptorsQty = (fileDescrEndBlock - fileDescrStartBlock + 1) * BLOCK_SIZE / sizeof(FileDescriptor);

    status = writeFileDescriptor(&fd, 0); // dir
    if (status < 0)
        return status;

    for (int i = 1; i < fileDescriptorsQty; ++i) {
        fd.length = -1;
        status = writeFileDescriptor(&fd, i);
        if (status < 0)
            return status;
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

    DirectoryEntry de;
    de.fileDescriptor = freeDescr;
    memcpy(de.filename, filename, FILE_NAME_SIZE);

    int status = writeDirectoryEntry(&de, freeDirEntry);
    if (status < 0)
        return status;

    FileDescriptor fd;
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

    DirectoryEntry de;
    int status = readDirectoryEntry(&de, dirEntry);
    if (status < 0)
        return status;

    const int descriptor = de.fileDescriptor;

    de.fileDescriptor = -1;
    memset(de.filename, 0, FILE_NAME_SIZE);

    status = writeDirectoryEntry(&de, dirEntry);
    if (status < 0)
        return status;

    status = removeFileDescriptor(descriptor);
    if (status < 0)
        return status;

    return 0;
}

int openFile(char filename[FILE_NAME_SIZE]) {
    const int dirEntry = findDirEntryByFileName(filename);
    if (dirEntry < 0)
        return dirEntry;

    DirectoryEntry de;
    int status = readDirectoryEntry(&de, dirEntry);
    if (status < 0)
        return status;

    int freeOftItemIndex = getFreeOftItemIndex();
    if (freeOftItemIndex < 0)
        return freeOftItemIndex;

    OpenFileTable* oftItem = &getFileTable()[freeOftItemIndex];
    oftItem->currPos = 0;
    oftItem->fileDescriptor = de.fileDescriptor;

    FileDescriptor fd;
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
    if (oftIndex < 0)
        return BAD_OFT_INDEX;

    OpenFileTable* oftItem = &getFileTable()[oftIndex];
    if (oftItem->currPos < 0)
        return -1;

    FileDescriptor fd;
    int status = readFileDescriptor(&fd, oftItem->fileDescriptor);
    if (status < 0)
        return status;

    status = writeBufferToDisk(oftItem, &fd);
    if (status < 0)
        return status;

    status = writeFileDescriptor(&fd, oftItem->fileDescriptor);
    if (status < 0)
        return status;

    oftItem->fileDescriptor = -1;
    oftItem->currPos = -1;

    return 0;
}

int read(int oftIndex, char* buffer, int count) {
    if (oftIndex < 0)
        return BAD_OFT_INDEX;

    OpenFileTable* oftItem = &getFileTable()[oftIndex];
    if (oftItem->fileDescriptor == -1)
        return FILE_NOT_OPENED;

    FileDescriptor fd;
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
    if (oftIndex < 0)
        return BAD_OFT_INDEX;

    OpenFileTable* oftItem = &getFileTable()[oftIndex];
    if (oftItem->fileDescriptor == -1)
        return FILE_NOT_OPENED;

    FileDescriptor fd;
    int status = readFileDescriptor(&fd, oftItem->fileDescriptor);
    if (status < 0)
        return status;

    int written = 0;
    char* pos = buffer;

    while (count - written > 0) {
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

int lseek(int oftIndex, int pos) {
    if (oftIndex < 0)
        return BAD_OFT_INDEX;

    OpenFileTable* oftItem = &getFileTable()[oftIndex];
    if (oftItem->fileDescriptor == -1)
        return FILE_NOT_OPENED;

    FileDescriptor fd;
    int status = readFileDescriptor(&fd, oftItem->fileDescriptor);
    if (status < 0)
        return status;

    if (pos >= fd.length)
        return -1;

    if (oftItem->currPos / BLOCK_SIZE != pos / BLOCK_SIZE) {
        int status = writeBufferToDisk(oftItem, &fd);
        if (status < 0)
            return status;

        oftItem->currPos = pos;
        status = readBufferFromDisk(oftItem, &fd);
        if (status < 0)
            return status;
    }
    else {
        oftItem->currPos = pos;
    }

    return 0;
}

int list(FileEntry** result) {
    const int entries = dirEntryCount();

    if (entries == 0)
        return 0;

    int found = 0;
    *result = malloc(entries * sizeof(FileEntry));

    DirectoryEntry de;
    FileDescriptor fd;

    for (int i = 0; i < dirEntryCount(); ++i) {
        int status = readDirectoryEntry(&de, i);
        if (status < 0)
            return status;

        if (de.fileDescriptor != -1) {
            status = readFileDescriptor(&fd, de.fileDescriptor);
            if (status < 0)
                return status;

            (*result)[found].descriptor = de.fileDescriptor;
            (*result)[found].length = fd.length;
            (*result)[found].name = malloc(FILE_NAME_SIZE + 1);
            memcpy((*result)[found].name, de.filename, FILE_NAME_SIZE);
            (*result)[found].name[FILE_NAME_SIZE] = '\0';

            ++found;
        }
    }

    if (found != entries) {
        *result = (FileEntry*)realloc(*result, found * sizeof(FileEntry));
    }

    return found;
}

/*
 *
 *
 *
 *
 */

