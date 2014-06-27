#include "driver_stub.h"

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

char* disk = 0;

int SECTOR_SIZE = 0;

int init(int cylinders, int surfaces, int sectors, int sector_size) {
    SECTOR_SIZE = sector_size;
    disk = (char*)malloc(cylinders * surfaces * sectors * sector_size);
    memset(disk, 0, cylinders * surfaces * sectors * sector_size);
    return 0;
}

int read_block(int block_number, char* buffer) {
    memcpy(buffer, disk + block_number * SECTOR_SIZE, SECTOR_SIZE);
    return 0;
}

int write_block(int block_number, char* buffer) {
    memcpy(disk + block_number * SECTOR_SIZE, buffer, SECTOR_SIZE);
    return 0;
}
