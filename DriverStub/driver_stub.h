#ifndef DRIVERSTUB_H
#define DRIVERSTUB_H

int init(int cylinders, int surfaces, int sectors, int sector_size);
int read_block(int block_number, char* buffer);
int write_block(int block_number, char* buffer);

#endif
