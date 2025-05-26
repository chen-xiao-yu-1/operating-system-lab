#include "fs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "block.h"
#include "log.h"
inode *root;     // 根目录inode
inode *cwd;      // 当前工作目录
int current_uid; // 当前用户ID
uint direntoff; // 用于确定dirent在块内的偏移量
uint direntblock; // 用于确定dirent所在的块号
void sbinit() {
    
    uchar buf[BSIZE];
    read_block(0, buf);
    memcpy(&sb, buf, sizeof(sb));
    
    root = iget(ROOTINO);
    if(root == NULL) {
        Error("sbinit: root inode does not exist");
        return;
    }
    
    cwd = iget(root->inum);
    
}

int cmd_f(int ncyl, int nsec) {
    get_disk_info(&ncyl, &nsec);
    
    // 计算文件系统参数
    sb.size = ncyl * nsec;        // 总块数
    sb.bmapstart = 1;            // 位图从第1块开始
    sb.magic=817; //确认是已经格式化的文件系统
    
    // 写入超级块
    uchar buf[BSIZE];
    memset(buf, 0, BSIZE);
    memcpy(buf, &sb, sizeof(sb));
    write_block(0, buf);
    
    memset(buf, 0, BSIZE);    
    // 标记超级块(0)和位图块(1)为已使用
    buf[0] = 0x3;  
    write_block(sb.bmapstart, buf);
    
    inode *root = ialloc(T_DIR);
    if(root == NULL) return E_ERROR;
    
    root->nlink = 1;
    root->owner_id = 0;      
    root->mode = 0755;       
    root->size = 0;
    root->type = T_DIR;
    root->atime = root->mtime = time(NULL);
    
    if(dirlink(root, ".", ROOTINO) < 0 || 
       dirlink(root, "..", ROOTINO) < 0) {  
        iput(root);
        return E_ERROR;
    }
    
    iupdate(root);
    iput(root);
    
    return E_SUCCESS;
}

int cmd_mk(char *name, short mode) {
    if(current_uid < 0) return E_NOT_LOGGED_IN;
    
    inode *ip = ialloc(T_FILE);
    if(ip == NULL) return E_ERROR;
    ip->mode = mode;
    ip->owner_id = current_uid;
    iupdate(ip);
    if(dirlink(cwd, name, ip->inum) < 0) {
        iput(ip);
        return E_ERROR;
    }
    
    iput(ip);
    return E_SUCCESS;
    
}
int cmd_mkdir(char *name, short mode) {
    if(current_uid < 0) return E_NOT_LOGGED_IN;

    if(lookup(cwd, name) != 0)
        return E_ERROR;
    
    inode *ip = ialloc(T_DIR);
    if(ip == NULL) return E_ERROR;
    
    ip->mode = mode | 0111;  
    ip->owner_id = current_uid;
    ip->nlink = 2;  
    ip->size = 0;
    ip->atime = ip->mtime = time(NULL);
    
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", cwd->inum) < 0) {
        iput(ip);
        return E_ERROR;
    }
    if(dirlink(cwd, name, ip->inum) < 0) {
        iput(ip);
        return E_ERROR;
    }
    
    iput(ip);
    return E_SUCCESS;
}

int cmd_rm(char *name) {
    if(current_uid < 0) return E_NOT_LOGGED_IN;
    
    inode *ip = namei(name);
    if(ip == NULL) return E_ERROR;
    
    if(ip->type == T_DIR) {
        iput(ip);
        return E_ERROR;
    }
     for(int i = 0; i < NDIRECT; i++) {
        if(ip->addrs[i]) {
            free_block(ip->addrs[i]);
            ip->addrs[i] = 0;
        }
        else{
            break;
        }
    }
   
    if (ip->indirect) {
    uchar buf[BSIZE];
    read_block(ip->indirect, buf); 

    uint *indirect_table = (uint *)buf;
    for (int i = 0; i < BSIZE / sizeof(uint); i++) {
        if (indirect_table[i]) {
            free_block(indirect_table[i]);
        }
    }

    free_block(ip->indirect);  
    ip->indirect = 0;
}

    if(ip->dindirect) {
        uchar buf1[BSIZE];
        read_block(ip->dindirect, buf1);
        uint *dindirect = (uint *)buf1;
        for(int i = 0; i < BSIZE / sizeof(uint); i++) {
            if(dindirect[i]) {
                uchar buf2[BSIZE];
                read_block(dindirect[i], buf2);
                uint *indirect = (uint *)buf2;
                for(int j = 0; j < BSIZE / sizeof(uint); j++) {
                    if(indirect[j])
                        free_block(indirect[j]);
                }
                free_block(dindirect[i]);
            }
        }
        free_block(ip->dindirect);
        ip->dindirect = 0;
    }
    
    ip->nlink--;
    if(ip->nlink == 0) {
        ip->type = 0;  
        free_block(ip->inum);  
    }
    iupdate(ip);
    iput(ip);

    //  从父目录中删除目录项
    uchar *old_buf = malloc(cwd->size);
    readi(cwd, old_buf, 0, cwd->size);
    uint pos=direntoff+direntblock*sizeof(struct dirent);
    uint len=sizeof(struct dirent);
    uchar *new_buf = malloc(cwd->size - len);
    memcpy(new_buf, old_buf, pos);                   
    memcpy(new_buf + pos, old_buf + pos + len, cwd->size - pos - len); // 复制后段
    writei(cwd, new_buf, 0, cwd->size - len); // 写入新内容
    free(old_buf);
    free(new_buf);
    if(deletei(cwd, pos, len) != E_SUCCESS) {
        iput(cwd);
        return E_ERROR;
    }
    cwd->nlink--;
    return E_SUCCESS;
    
}

int cmd_cd(char *path) {
    if(current_uid < 0) 
        return E_NOT_LOGGED_IN;
        
    if(path == NULL || path[0] == '\0')
        return E_SUCCESS;
        
    inode *ip;
    inode *old_cwd = iget(cwd->inum);
    if(path[0] == '/') {
        iput(cwd);  
        cwd = iget(root->inum);     
        path++;            
        if(*path == '\0') { 
        iput(old_cwd);
        return E_SUCCESS;
        }
    }    
   
    char name[MAXNAME];
    char *start = path;
    char *end;
    while(1) {
       
        end = strchr(start, '/');
        if(end == NULL) {
           
            if(strlen(start) >= MAXNAME)
                return E_INVALID_NAME;
            strcpy(name, start);
        } else {
            if(end - start >= MAXNAME)
                return E_INVALID_NAME;
            strncpy(name, start, end - start);
            name[end - start] = '\0';
        }

        if(name[0] == '\0') {
            if(end == NULL) break;
            start = end + 1;
            continue;
        }
        
        if(strcmp(name, ".") == 0) {
            if(end == NULL) break;
            start = end + 1;
            continue;
        }
        
        if(strcmp(name, "..") == 0) {
            ip = namei(name);
            if(ip == NULL) {
            iput(ip);
            return E_ERROR;  
        }
        iput(cwd);
        cwd=ip;
            if(end == NULL) break;
            start = end + 1;
            continue;
        }
        
        ip = namei(name);
        if(ip == NULL) {
            iput(ip);
            iput(cwd);
            cwd = old_cwd;
            return E_ERROR;  
        }
        iput(cwd);
        cwd=ip;
        if(end == NULL) break;
        start = end + 1;
    }
    iput(old_cwd);
    return E_SUCCESS;
}

int cmd_rmdir(char *name) {
    if (current_uid < 0) return E_NOT_LOGGED_IN;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return E_INVALID_NAME;

    inode *ip = namei(name);
    if (ip == NULL) return E_DIR_NOT_FOUND;

    if (ip->type != T_DIR) {
        iput(ip);
        return E_NOT_DIRECTORY;
    }

    inode *old_cwd = cwd;
    cwd = ip;

    for (int i = 0; i < NDIRECT; i++) {
    if (ip->addrs[i] == 0) continue;

    uint blockno = ip->addrs[i];
    uchar buf[BSIZE];                      
    read_block(blockno, buf) ;   
       

    struct dirent *entries = (struct dirent *)buf;

    for (int j = 0; j < BSIZE / sizeof(struct dirent); j++) {
        struct dirent *de = &entries[j];
        if (de->inum == 0) continue;
        if (strcmp(de->name, ".") == 0 || strcmp(de->name, "..") == 0)
            continue;

        inode *child_ip = iget(de->inum);
        if (child_ip == NULL) continue;

        if (child_ip->type == T_DIR) {
            iput(child_ip);
            int res = cmd_rmdir(de->name);
            if (res != E_SUCCESS) {
                cwd = old_cwd;
                iput(ip);
                return res;
            }
        } else {
            iput(child_ip);
            int res = cmd_rm(de->name);
            if (res != E_SUCCESS) {
                cwd = old_cwd;
                iput(ip);
                return res;
            }
        }
    }
}   

    free_block(ip->inum);  
    iput(ip);  

    cwd = old_cwd;

    if(lookup(cwd, name)==0) {
        iput(cwd);
        return E_ERROR;
    }
    uchar *old_buf = malloc(cwd->size);
    readi(cwd, old_buf, 0, cwd->size);
    uint pos=direntoff+direntblock*sizeof(struct dirent);
    uint len=sizeof(struct dirent);
    uchar *new_buf = malloc(cwd->size - len);
    memcpy(new_buf, old_buf, pos);                   
    memcpy(new_buf + pos, old_buf + pos + len, cwd->size - pos - len); // 复制后段
    writei(cwd, new_buf, 0, cwd->size - len); 
    free(old_buf);
    free(new_buf);
    if(deletei(cwd, pos, len) != E_SUCCESS) {
        iput(cwd);
        return E_ERROR;
    }
    cwd->nlink--;
    return E_SUCCESS;
}

int cmd_ls(entry **entries, int *n) {
    if(current_uid < 0) return E_NOT_LOGGED_IN;

    // 先统计有效目录项数量（跳过 "." 和 ".."）
    *n = 0;
    uint off;
    struct dirent de;

    for(off = 0; off < cwd->size; off += sizeof(de)) {
        if(readi(cwd, (uchar*)&de, off, sizeof(de)) != sizeof(de))
            break;
        if(de.inum != 0 && strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0)
            (*n)++;
    }
    *entries = malloc(sizeof(entry) * (*n));
    if(*entries == NULL)
        return E_ERROR;

    int i = 0;
    for(off = 0; off < cwd->size; off += sizeof(de)) {
        if(readi(cwd, (uchar*)&de, off, sizeof(de)) != sizeof(de))
            break;
        if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            continue;

        inode *ip = iget(de.inum);
        if(ip == NULL)
            continue;

        (*entries)[i].type = ip->type;
        (*entries)[i].inum = ip->inum;
        (*entries)[i].size = ip->size;
        (*entries)[i].mode = ip->mode;
        (*entries)[i].mtime = ip->mtime;
        memcpy((*entries)[i].name, de.name, MAXNAME);

        iput(ip);
        i++;
    }

    return E_SUCCESS;
}

int cmd_cat(char *name, uchar **buf, uint *len) {
    if(current_uid < 0) return E_NOT_LOGGED_IN;
    
    inode *ip = namei(name);
    if(ip == NULL) return E_FILE_NOT_FOUND;

    *len = ip->size;
    *buf = malloc(*len);
    if(*buf == NULL) {
        iput(ip);
        return E_ERROR;
    }
    
    if(readi(ip, *buf, 0, *len) != *len) {
        free(*buf);
        iput(ip);
        return E_ERROR;
    }
    
    ip->atime = time(NULL);
    iupdate(ip);
    iput(ip);
    return E_SUCCESS;
}
 inode* namei(char *name) {
    if(name == NULL || cwd == NULL)
        return NULL;
        
    uint inum = lookup(cwd, name);
    if(inum == 0)
        return NULL;  
    return iget(inum);
}
int cmd_w(char *name, uint len, const char *data) {
    if(current_uid < 0) return E_NOT_LOGGED_IN;
    
    inode *ip = namei(name);
    if(ip == NULL) return E_ERROR;
    if(writei(ip, (uchar*)data, 0, len) != len) {
        iput(ip);
        return E_ERROR;
    }
    
    ip->mtime = time(NULL);
    iupdate(ip);
    iput(ip);
    return E_SUCCESS;
}

int cmd_i(char *name, uint pos, uint len, const char *data) {
    if(current_uid < 0) return E_NOT_LOGGED_IN;
    
    inode *ip = namei(name);
    if(ip == NULL) return E_ERROR;
    
    if(pos > ip->size) {
        iput(ip);
        return E_ERROR;
    }
    
    uchar *old_buf = NULL;
    cmd_cat(name, &old_buf, &ip->size);
    uchar *new_buf = malloc(ip->size + len);
    memcpy(new_buf, old_buf, pos);                    
    memcpy(new_buf + pos, data, len);                
    memcpy(new_buf + pos + len, old_buf + pos, ip->size - pos); 
    writei(ip, new_buf, 0, ip->size + len); 
    free(old_buf);
    free(new_buf);
  
    ip->mtime = time(NULL);
    iupdate(ip);
    iput(ip);
    return E_SUCCESS;
}
int cmd_d(char *name, uint pos, uint len) {
    if(current_uid < 0) return E_NOT_LOGGED_IN;
    
    inode *ip = namei(name);
    if(ip == NULL) return E_FILE_NOT_FOUND;
    if(pos + len > ip->size) {
        iput(ip);
        return E_INVALID_RANGE;
    }  
    uchar *old_buf = NULL;
    cmd_cat(name, &old_buf, &ip->size);
    uchar *new_buf = malloc(ip->size - len); 
    memcpy(new_buf + pos, old_buf + pos + len, ip->size - pos - len); // 复制后段
    writei(ip, new_buf, 0, ip->size - len); 
    free(old_buf);
    free(new_buf);
    if(deletei(ip, pos, len) != E_SUCCESS) {
        iput(ip);
        return E_ERROR;
    }
    iput(ip);
    return E_SUCCESS;
}

int cmd_login(int auid) {
    
    if(auid < 0)
        return E_INVALID_USER;
    current_uid = auid;
    return E_SUCCESS;
}

 uint lookup(inode *dp, char *name) {
    struct dirent de;
    uint off;
    if(dp == NULL || name == NULL || dp->type != T_DIR)
        return 0;
    direntblock = 0;
    for(off = 0; off < dp->size; off += sizeof(de)) {
        uint block_no;  
        uint block_off; 
        uchar buf[BSIZE];
        
        block_no = bmap(dp, off / BSIZE);  
        if(block_no == 0)
            continue;
        block_off = off % BSIZE;
        read_block(block_no, buf);
        memmove(&de, buf + block_off, sizeof(de));
        if(de.inum != 0 && strcmp(name, de.name) == 0){
            direntblock=off / BSIZE;
            direntoff = block_off;
            return de.inum;
    }
}
    return 0;  
}
 int dirlink(inode *dp, char *name, uint inum) {
    struct dirent de;
    uint off;
    if(dp == NULL || name == NULL || dp->type != T_DIR)
        return -1;
    if(lookup(dp, name) != 0)
        return -1;  // 已存在同名文件/目录
        
    // 查找空闲目录项或添加到末尾
    for(off = 0; off < dp->size; off += sizeof(de)) {
        uint block_no = bmap(dp, off / BSIZE);
        if(block_no == 0)
            return -1;
            
        uchar buf[BSIZE];
        read_block(block_no, buf);    
        memmove(&de, buf + (off % BSIZE), sizeof(de));
        
        if(de.inum == 0)  // 找到空闲目录项
            break;
    }
    de.inum = inum;
    strncpy(de.name, name, MAXNAME);
    if(writei(dp, (uchar*)&de, off, sizeof(de)) != sizeof(de))
        return -1;      
    return 0;
}

void fs_shutdown() {
    if (cwd) iput(cwd);  
    if (root != NULL && root != cwd) iput(root);
    cwd = NULL;
    root = NULL;
}
