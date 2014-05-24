#ifndef FAKEDRIVER_H
#define FAKEDRIVER_H

int init(int blocks, int blockSize);
int read_block(int i, char *p);
int write_block(int i, char *p);
void dump_disk();

#endif
