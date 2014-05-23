#ifndef DEFINES_H
#define DEFINES_H

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

#define FILE_TABLE_SIZE 10

#endif
