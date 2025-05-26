#ifndef __BLOCK_H__
#define __BLOCK_H__

#include "common.h"

typedef struct {
    uint magic;      
    uint size;
    uint bmapstart;
    uint rootinum;
   
} superblock;

extern superblock sb;

void zero_block(uint bno);
uint allocate_block();
void free_block(uint bno);

int get_disk_info(int *ncyl, int *nsec);
void read_block(int blockno, uchar *buf);
void write_block(int blockno, uchar *buf);

#endif