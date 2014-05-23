#include "fake_driver.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char* DISK = 0;

int blocksInternal = 1;
int blockSizeInternal = 1;


int init(int blocks, int blockSize) {
    blocksInternal = blocks;
    blockSizeInternal = blockSize;

    DISK = (char*)malloc(blockSize * blocks);

    return 0;
}

int read_block(int i, char *p) {
    memcpy(p, DISK + blockSizeInternal * i, blockSizeInternal);
    return 0;
}

int write_block(int i, char *p) {
    memcpy(DISK + blockSizeInternal * i, p, blockSizeInternal);
    return 0;
}

void dump_disk() {
    for (int b = 0; b < blocksInternal; ++b) {
        for (int i = 0; i < blockSizeInternal; ++i) {
            printf("%d ", DISK[b * blockSizeInternal + i]);
        }
        printf("\n");
    }
}
