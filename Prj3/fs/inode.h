#ifndef __INODE_H__
#define __INODE_H__

#include "common.h"
#include <time.h> 
#define NDIRECT 10  
#define NINDIRECT (BSIZE / sizeof(uint))  
#define MAXNAME 28//凑整，防止栈溢出
enum {
    T_DIR = 1,   
    T_FILE = 2,  
};

typedef struct {
    uint inum;               
    ushort type;             
    uint nlink;  // Number of links
    uint owner_id; 
    uint group_id; 
    uint atime;    // Last access time
    uint mtime;    // Last modification time            
    uint size;                // File Size in bytes
    uint blocks;              // Number of blocks, may be larger than size
    uint mode;
    uint addrs[NDIRECT];  // Data block addresses,
    uint indirect;                           // the last two are indirect blocks
    uint dindirect;  
} dinode;//88B
typedef struct {
    uint inum;                 
    int valid;                
    int ref;                  
    int dirty;               
  
    ushort type;              
    uint size;                
    uint blocks;              
    uint nlink;              
    uint owner_id;           
    uint mode;                
    uint atime;              
    uint mtime;               
    uint addrs[NDIRECT];  
    uint indirect;         
    uint dindirect;           
} inode;// 92B

inode *iget(uint inum);

void iput(inode *ip);

inode *ialloc(short type);

void iupdate(inode *ip);

int readi(inode *ip, uchar *dst, uint off, uint n);

int writei(inode *ip, uchar *src, uint off, uint n);

uint bmap(inode *ip, uint bn);

uint deletei(inode *ip,uint pos,uint len);
#endif
