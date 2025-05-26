#include "block.h"

#include <string.h>

#include "common.h"
#include "log.h"

#define MAX_BLOCKS 1024  
uchar disk[MAX_BLOCKS][BSIZE];  // 模拟磁盘空间
void init_disk() {
    memset(disk, 0, sizeof(disk));
}

superblock sb;
static void mark_block_used(uint bno) {
    uchar buf[BSIZE];
    read_block(sb.bmapstart, buf);
    buf[bno/8] |= (1 << (bno % 8));
    write_block(sb.bmapstart, buf);
}

static void mark_block_free(uint bno) {
    uchar buf[BSIZE];
    read_block(sb.bmapstart, buf);
    buf[bno/8] &= ~(1 << (bno % 8));
    write_block(sb.bmapstart, buf);
}
void zero_block(uint bno) {
    uchar buf[BSIZE];
    memset(buf, 0, BSIZE);
    write_block(bno, buf);
}

static int is_block_used(uint bno) {
    uchar buf[BSIZE];
    read_block(sb.bmapstart, buf);
    return (buf[bno/8] & (1 << (bno % 8))) != 0;
}


uint allocate_block() {
    uchar buf[BSIZE];
    read_block(sb.bmapstart, buf);
    
    for (uint b = 2; b < sb.size; b++) {
        if(!is_block_used(b)) {
            mark_block_used(b);
            zero_block(b);
            return b;
        }
    }
    
    Warn("Out of blocks");
    return 0;
}

void free_block(uint bno) {
    if(bno >= sb.size || bno < 2) {
        Error("free_block: invalid block number");
        return;
    }
    if(!is_block_used(bno)) {
        Error("free_block: block already free");
        return;
    }
    mark_block_free(bno);
    zero_block(bno);  
}
void get_disk_info(int *ncyl, int *nsec) {
    *ncyl=16;
    *nsec=64;
    sb.size = (*ncyl) * (*nsec);  
    
    if(sb.size > MAX_BLOCKS) {
        Error("Disk size exceeds maximum supported size");
        sb.size = MAX_BLOCKS;
    }
}

void read_block(int blockno, uchar *buf) {
    if(blockno >= sb.size) {
        Error("read_block: block number out of range");
        return;
    }
    memcpy(buf, disk[blockno], BSIZE); 
}

void write_block(int blockno, uchar *buf) {
    if(blockno >= sb.size) {
        Error("write_block: block number out of range");
        return;
    }
    memcpy(disk[blockno], buf, BSIZE);
    
    
}
