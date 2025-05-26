#ifndef __BLOCK_H__
#define __BLOCK_H__

#include "common.h"

typedef struct {
    uint magic;      // Magic number, used to identify the file system
    uint size;       // Size in blocks
    uint bmapstart;  // Block number of first free map block
} superblock;

extern superblock sb;

void zero_block(uint bno);
uint allocate_block();
void free_block(uint bno);

void get_disk_info(int *ncyl, int *nsec);
void read_block(int blockno, uchar *buf);
void write_block(int blockno, uchar *buf);

#endif