#include "inode.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "block.h"
#include "log.h"

inode *iget(uint inum) {
    uchar buf[BSIZE];
    read_block(inum, buf);
    if(inum >= sb.size || inum < 2) {
        Error("iget: invalid inode number");
        return NULL;
    }   
    dinode *dip = (dinode*)buf;
    if(dip->type == 0) {
        return NULL;
    }
    inode *ip = malloc(sizeof(inode));
    if(ip == NULL) return NULL;
    ip->inum = inum;
    ip->ref = 1;
    ip->valid = 1;
    ip->dirty = 0;

    ip->type = dip->type;
    ip->size = dip->size;
    ip->blocks = dip->blocks;
    ip->nlink = dip->nlink;
    ip->owner_id = dip->owner_id;
    ip->mode = dip->mode;
    ip->atime = dip->atime;
    ip->mtime = dip->mtime;
    memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
    ip->indirect = dip->indirect;
    ip->dindirect = dip->dindirect;
    
    return ip;
}
    void iput(inode *ip) {
        if(ip == NULL)
            return;
        ip->ref--;
        if(ip->ref == 0) {
            if(ip->dirty) {
                iupdate(ip);
            }
            free(ip);
        }
    }
    inode *ialloc(short type) {
        uchar buf[BSIZE];
        uint block = allocate_block();
        if(block == 0) {
            Error("ialloc: no free blocks");
            return NULL;
        }

        dinode *dip = (dinode*)buf;
        memset(dip, 0, sizeof(*dip));
        dip->inum= block;
        dip->type = type;
        dip->nlink = 1;
        dip->owner_id = 0;        
        dip->group_id = 0;         
        dip->atime =time(NULL); 
        dip->mtime = dip->atime;   
        dip->size = 0;             
        dip->blocks = 0;           
        dip->mode = 0666;          
        memset(dip->addrs, 0, sizeof(dip->addrs));  
        dip->indirect = 0;       
        dip->dindirect = 0;        
        write_block(block, buf);
        inode *ip = (inode*)malloc(sizeof(inode));
        if(ip == NULL) {
            free_block(block);
            return NULL;
        }
        memset(ip, 0, sizeof(*ip));
        ip->inum = block;  
        ip->type = type;
        ip->nlink = 1;
        ip->ref = 1;
        ip->valid = 1;
        ip->size = 0;
        ip->blocks = 0;
        ip->dirty = 0;
              
        return ip;
    }
    
    void iupdate(inode *ip) {
        uchar buf[BSIZE];
        dinode *dip;
        read_block(ip->inum, buf);
        dip = (dinode*)buf;
        dip->type = ip->type;
        dip->size = ip->size;
        dip->blocks = ip->blocks;
        dip->nlink = ip->nlink;
        dip->owner_id = ip->owner_id;
        dip->mode = ip->mode;
        dip->atime = ip->atime;
        dip->mtime = ip->mtime;
        memmove(dip->addrs, ip->addrs, sizeof(dip->addrs));
       dip->indirect = ip->indirect;
        dip->dindirect = ip->dindirect;
        
        write_block(dip->inum, buf); 
        ip->dirty = 0;
    }

uint bmap(inode *ip, uint bn) {
    //此处bn是文件逻辑块号
    uint addr, *a;
    uchar buf[BSIZE];
    
    if(bn < NDIRECT) {
        if((addr = ip->addrs[bn]) == 0) { 

            addr = allocate_block();  
            if(addr == 0)
                return 0;
            ip->addrs[bn] = addr;
            ip->blocks++;
            ip->dirty = 1;
        }
        return addr;
    }
    bn -= NDIRECT;

    if(bn < NINDIRECT) {
        if((addr = ip->indirect) == 0) {
            addr = allocate_block();
            if(addr == 0)
                return 0;
            ip->indirect = addr;
            ip->blocks++; 
            ip->dirty = 1;
        }
        read_block(addr, buf);
        a = (uint*)buf;
        if((addr = a[bn]) == 0) {
            addr = allocate_block();
            if(addr == 0)
                return 0;
            a[bn] = addr;
            ip->blocks++;
            write_block(ip->indirect, buf);
        }
        return addr;
    }
    bn -= NINDIRECT;

    if(bn < NINDIRECT * NINDIRECT) {
        if((addr = ip->dindirect) == 0) {
            addr = allocate_block();
            if(addr == 0)
                return 0;
            ip->dindirect = addr;
            ip->blocks++;
            ip->dirty = 1;
        }

        read_block(addr, buf);
        a = (uint*)buf;
        
        uint index = bn / NINDIRECT;
        if((addr = a[index]) == 0) {
            addr = allocate_block();
            if(addr == 0)
                return 0;
            a[index] = addr;
            ip->blocks++;
            write_block(ip->dindirect, buf);
        }
        
        read_block(addr, buf);
        a = (uint*)buf;
        
        if((addr = a[bn % NINDIRECT]) == 0) {
            addr = allocate_block();
            if(addr == 0)
                return 0;
            a[bn % NINDIRECT] = addr;
            ip->blocks++;
            write_block(addr, buf);
        }
        return addr;
    }
    Error("bmap: block number too large");
    return 0;
}

int readi(inode *ip, uchar *dst, uint off, uint n) {
    uint tot, m;
    uchar buf[BSIZE];
    
    if(off > ip->size || off + n < off)
        return -1;
    if(off + n > ip->size)
        n = ip->size - off;
        
    for(tot = 0; tot < n; tot += m, off += m, dst += m) {
        uint block = bmap(ip, off/BSIZE);
        if(block == 0)
            return -1;
        m = min(n - tot, BSIZE - off%BSIZE);
        read_block(block, buf);
        memmove(dst, buf + off%BSIZE, m);
    }
    
    return n;
}

int writei(inode *ip, uchar *src, uint off, uint n) {
    uint tot, m;
    uchar buf[BSIZE];
    
    if(off > ip->size || off + n < off)
        return -1;
        
    for(tot = 0; tot < n; tot += m, off += m, src += m) {
        uint block = bmap(ip, off/BSIZE);
        if(block == 0)
            return -1;
        m = min(n - tot, BSIZE - off%BSIZE);
        if(m < BSIZE) {
            read_block(block, buf);
        }
        memmove(buf + off%BSIZE, src, m);
        write_block(block, buf);
    }
    
    if(off > ip->size) {
        ip->size = off;
        ip->dirty = 1;
    }
    ip->mtime = time(NULL);
    iupdate(ip);
    return n;
}

uint deletei(inode *ip,uint pos,uint len){
    
    uint old_blocks = (ip->size + BSIZE - 1) / BSIZE;
    uint new_blocks = ((ip->size - len) + BSIZE - 1) / BSIZE;
for (uint b = new_blocks; b < old_blocks && b < NDIRECT; b++) {
    if (ip->addrs[b]) {
        free_block(ip->addrs[b]); 
        ip->addrs[b] = 0;    
    }
}
    if (old_blocks > NDIRECT) {
        uint indirect_blocks_to_clear = old_blocks - NDIRECT;
        uint indirect_start = new_blocks > NDIRECT ? new_blocks - NDIRECT : 0;

        uchar buf[BSIZE];
        read_block(ip->indirect, buf);  
        uint *indirect = (uint *)buf;

    for (uint i = indirect_start; i < indirect_blocks_to_clear; i++) {
        if (indirect[i]) {
            free_block(indirect[i]);
            indirect[i] = 0;
        }
    }

    bool all_zero = true;
    for (uint i = 0; i < NINDIRECT; i++) {
        if (indirect[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (all_zero) {
        free_block(ip->indirect);
        ip->indirect = 0;
    } else {
        inode *indirect_ip = iget(ip->indirect);
        writei(indirect_ip, buf, 0, BSIZE); 
        iput(indirect_ip); 
    }
}
    ip->size -= len;
    ip->mtime = time(NULL);
    iupdate(ip);
    return E_SUCCESS;
}