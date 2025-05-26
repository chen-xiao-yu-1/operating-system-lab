#include "fs.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "block.h"
#include "log.h"

inode *root;     
inode *cwd;      
uint direntoff; 
uint direntblock; 
User users[MAXUSERS];

int check_read_permission(inode *ip, int uid) {
    if(ip == NULL)
        return 0;
    if(uid == 0)
        return 1;
        
    if(ip->owner_id == (uint)uid)
        return (ip->mode & 0400) != 0;  

    return (ip->mode & 0004) != 0; 
}
int check_write_permission(inode *ip, int uid) {
    if(ip == NULL)
        return 0;
        
    if(uid == 0)
        return 1;
        
    if(ip->owner_id == (uint)uid)
       return (ip->mode & 0200) != 0; 

    return (ip->mode & 0002) != 0; 
}

void sbinit() {
    
    uchar buf[BSIZE];
    read_block(0, buf);
    memcpy(&sb, buf, sizeof(sb));
    root = iget(sb.rootinum);
    if(root == NULL) {
        Error("sbinit: root inode does not exist");
        return;
    }
    cwd = iget(root->inum);
    
}

int cmd_f(int ncyl, int nsec,int client_id) {
    if(users[client_id].uid !=0) return E_ERROR;
    get_disk_info(&ncyl, &nsec);
    sb.size = ncyl * nsec;        
    sb.bmapstart = 1;            
    sb.magic=817;
    uchar buf[BSIZE];

    memset(buf, 0, BSIZE);    
    buf[0] = 0x3;  
    write_block(sb.bmapstart, buf);

    inode *root = ialloc(T_DIR);
    if(root == NULL) return E_ERROR;
    root->nlink = 1;
    root->owner_id = 0;      
    root->mode = 0777;       
    root->size = 0;
    root->type = T_DIR;
    root->atime = root->mtime = time(NULL);
    sb.rootinum = root->inum;  
    if(dirlink(root, ".", ROOTINO,root->owner_id) < 0 || 
       dirlink(root, "..", ROOTINO,root->owner_id) < 0) {  
        iput(root);
        return E_ERROR;
    }
    char buf1[BSIZE];
    iupdate(root);
    iput(root);
    
    memset(buf, 0, BSIZE);
    memcpy(buf, &sb, sizeof(sb));
    write_block(0, buf);
    return E_SUCCESS;
}

int cmd_mk(int client_id,char *name, short mode) {
    if(users[client_id].uid< 0) return E_NOT_LOGGED_IN;
    inode *ip = ialloc(T_FILE);
    if(ip == NULL) return E_ERROR;
    ip->mode = mode;
    ip->atime = ip->mtime = time(NULL);
    iput(cwd);
    cwd = iget(users[client_id].current_dir);
    ip->owner_id = users[client_id].uid;
    
    iupdate(ip);
    if(dirlink(cwd, name, ip->inum,client_id) < 0) {
        iput(ip);
        return E_ERROR;
    }
    
    iput(ip);
    return E_SUCCESS;
    
}
int cmd_mkdir(int client_id, char *name, short mode) {
    if (users[client_id].uid < 0) return E_NOT_LOGGED_IN;
    if(cwd->owner_id != users[client_id].uid&&cwd!=root) {
        iput(cwd);
        cwd = iget(users[client_id].current_dir);
    }
    if(lookup(cwd, name,client_id) != 0)
        return E_ERROR;
    
    inode *ip = ialloc(T_DIR);
    if(ip == NULL) return E_ERROR;
    
    ip->mode = mode | 0001;  
    ip->owner_id = users[client_id].uid;
    ip->nlink = 2;  
    ip->size = 0;
    ip->atime = ip->mtime = time(NULL);
    
    if(dirlink(ip, ".", ip->inum,client_id) < 0 || dirlink(ip, "..", cwd->inum,client_id) < 0) {
        iput(ip);
        return E_ERROR;
    }
    
    if(dirlink(cwd, name, ip->inum,client_id) < 0) {
        iput(ip);
        return E_ERROR;
    }
    
    iput(ip);
    return E_SUCCESS;
}

int cmd_rm(int client_id,char *name) {
    if (users[client_id].uid < 0) return E_NOT_LOGGED_IN;
    if(cwd->owner_id != users[client_id].uid&&cwd!=root) {
    iput(cwd);
    cwd = iget(users[client_id].current_dir);}
    inode *ip = namei(name,client_id);
    if(ip == NULL) return E_ERROR;
    
    // 2. 检查权限
    if(!check_write_permission(ip, users[client_id].uid)) {
        iput(ip);
        return E_PERMISSION_DENIED;
    }
    

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
    for (uint i = 0; i < BSIZE / sizeof(uint); i++) {
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
        for(uint i = 0; i < BSIZE / sizeof(uint); i++) {
            if(dindirect[i]) {
                uchar buf2[BSIZE];
                read_block(dindirect[i], buf2);
                uint *indirect = (uint *)buf2;
                for(uint j = 0; j < BSIZE / sizeof(uint); j++) {
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
    if(root==NULL){
        root=iget(ROOTINO);
    }
    return E_SUCCESS;
}

int cmd_cd(int client_id, char *path) {
    if (users[client_id].uid < 0) return E_NOT_LOGGED_IN;
    if(cwd->owner_id != users[client_id].uid&&cwd!=root) {
        iput(cwd);
        cwd = iget(users[client_id].current_dir);
    }   
    if(path == NULL || path[0] == '\0')
        return E_SUCCESS;
        
    inode *ip;
    inode *old_cwd = iget(cwd->inum);
    if(path[0] == '/') {
        iput(cwd);  
        cwd = iget(root->inum); 
        users[client_id].current_dir = cwd->inum; 
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
            ip = namei(name,client_id);
            if(ip == NULL) {
            iput(ip);
            return E_ERROR;  
        }
        iput(cwd);
        cwd=ip;
        users[client_id].current_dir = cwd->inum;
            if(end == NULL) break;
            start = end + 1;
            continue;
        }
        
        ip = namei(name,client_id);
        if(ip == NULL) {
            iput(ip);
            iput(cwd);
            cwd = old_cwd;
            users[client_id].current_dir = cwd->inum;
            return E_ERROR;  
        }
        iput(cwd);
        cwd=ip;
        users[client_id].current_dir = cwd->inum;
        if(end == NULL) break;
        start = end + 1;
    }
    iput(old_cwd);
    return E_SUCCESS;
}

int cmd_rmdir( int client_id,char *name) {
    if (users[client_id].uid < 0) return E_NOT_LOGGED_IN;

    if(cwd->owner_id != users[client_id].uid&&cwd!=root) {
    iput(cwd);
    cwd = iget(users[client_id].current_dir);}
    //  检查权限
    if(!check_write_permission(cwd, users[client_id].uid)) {
        return E_PERMISSION_DENIED;
    }
    
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return E_INVALID_NAME;

    inode *ip = namei(name,client_id);
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

    for (uint j = 0; j < BSIZE / sizeof(struct dirent); j++) {
        struct dirent *de = &entries[j];
        if (de->inum == 0) continue;
        if (strcmp(de->name, ".") == 0 || strcmp(de->name, "..") == 0)
            continue;

        inode *child_ip = iget(de->inum);
        if (child_ip == NULL) continue;

        if (child_ip->type == T_DIR) {
            iput(child_ip);
            int res = cmd_rmdir(client_id,de->name);
            if (res != E_SUCCESS) {
                cwd = old_cwd;
                iput(ip);
                return res;
            }
        } else {
            iput(child_ip);
            int res = cmd_rm(client_id,de->name);
            if (res != E_SUCCESS) {
                cwd = old_cwd;
                iput(ip);
                return res;
            }
        }
    }
}   
    cwd = old_cwd;
    if(lookup(cwd, name,client_id)==0) {
        iput(cwd);
       return E_ERROR;
    }
    free_block(ip->inum);  
    iput(ip);  
    uchar *old_buf = malloc(cwd->size);
    readi(cwd, old_buf, 0, cwd->size);
    uint pos=direntoff+direntblock*sizeof(struct dirent);
    uint len=sizeof(struct dirent);
    uchar *new_buf = malloc(cwd->size - len);
    memcpy(new_buf, old_buf, pos);                    // 复制前段
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


int cmd_ls(int client_id,entry **entries, int *n) {
    if (users[client_id].uid < 0) return E_NOT_LOGGED_IN;
    if(cwd->owner_id != users[client_id].uid&&cwd!=root) {
        iput(cwd);
        cwd = iget(users[client_id].current_dir);}

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
        (*entries)[i].owner_id=ip->owner_id;
        strncpy((*entries)[i].name, de.name, MAXNAME);

        iput(ip);
        i++;
    }

    return E_SUCCESS;
}

int cmd_cat(int client_id,char *name, uchar **buf, uint *len) {
    if (users[client_id].uid < 0) return E_NOT_LOGGED_IN;
    if(cwd->owner_id != users[client_id].uid&&cwd!=root) {
        iput(cwd);
        cwd = iget(users[client_id].current_dir);}
    
    inode *ip = namei(name,client_id);
    if(ip == NULL) return E_FILE_NOT_FOUND;
    
    if(!check_read_permission(ip, users[client_id].uid)) {
        iput(ip);
        return E_PERMISSION_DENIED;
    }
    
    // 3. 读取文件内容
    *len = ip->size;
    *buf = malloc(*len);
    if(*buf == NULL) {
        iput(ip);
        return E_ERROR;
    }
    
    if(readi(ip, *buf, 0, *len) != (int)*len) {
        free(*buf);
        iput(ip);
        return E_ERROR;
    }
    
    ip->atime = time(NULL);
    iupdate(ip);
    iput(ip);
    return E_SUCCESS;
}
 inode* namei(char *name ,int client_id) {
    if(name == NULL || cwd == NULL)
        return NULL;
        
    uint inum = lookup(cwd, name,client_id);
    if(inum == 0)
        return NULL;  
    return iget(inum);
}
int cmd_w(int client_id,char *name, uint len, const char *data) {
    if (users[client_id].uid < 0) return E_NOT_LOGGED_IN;
    if(cwd->owner_id != users[client_id].uid&&cwd!=root) {
        iput(cwd);
        cwd = iget(users[client_id].current_dir);}
    inode *ip = namei(name,client_id);
    if(ip == NULL) return E_ERROR;
    

    if(!check_write_permission(ip, users[client_id].uid)) {
        iput(ip);
        return E_PERMISSION_DENIED;
    }

    // 3. 写入数据
    if(writei(ip, (uchar*)data, 0, len) != (int)len) {
        iput(ip);
        return E_ERROR;
    }
    
    // 4. 更新时间戳
    ip->mtime = time(NULL);
    iupdate(ip);
    iput(ip);
    return E_SUCCESS;
}

int cmd_i(int client_id,char *name, uint pos, uint len, const char *data) {
    if (users[client_id].uid < 0) return E_NOT_LOGGED_IN;
    if(cwd->owner_id != users[client_id].uid&&cwd!=root) {
        iput(cwd);
        cwd = iget(users[client_id].current_dir);}
    // 1. 查找文件
    inode *ip = namei(name,client_id);
    if(ip == NULL) return E_ERROR;
    
    // 2. 检查权限

    if(!check_write_permission(ip, users[client_id].uid)) {
        iput(ip);
        return E_PERMISSION_DENIED;
    }
    // 3. 检查位置是否有效
    if(pos > ip->size) {
        iput(ip);
        return E_ERROR;
    }
    
    uchar *old_buf = NULL;
    cmd_cat(client_id,name, &old_buf, &ip->size);
    uchar *new_buf = malloc(ip->size + len);
    memcpy(new_buf, old_buf, pos);                    // 复制前段
    memcpy(new_buf + pos, data, len);                 // 插入新内容
    memcpy(new_buf + pos + len, old_buf + pos, ip->size - pos); // 复制后段
    writei(ip, new_buf, 0, ip->size + len); // 写入新内容
    free(old_buf);
    free(new_buf);
    // 5. 更新inode

    ip->mtime = time(NULL);
    iupdate(ip);
    iput(ip);
    return E_SUCCESS;
}
int cmd_d(int client_id,char *name, uint pos, uint len) {
    if (users[client_id].uid < 0) return E_NOT_LOGGED_IN;
    iput(cwd);
    cwd = iget(users[client_id].current_dir);
    // 1. 查找文件
    inode *ip = namei(name,client_id);
    if(ip == NULL) return E_FILE_NOT_FOUND;
    
    // 2. 检查权限

    if(!check_write_permission(ip, users[client_id].uid)) {
        iput(ip);
        return E_PERMISSION_DENIED;
    }
    // 3. 检查范围是否有效
    if(pos + len > ip->size) {
        iput(ip);
        return E_INVALID_RANGE;
    }  
    uchar *old_buf = NULL;
    cmd_cat(client_id,name, &old_buf, &ip->size);
    uchar *new_buf = malloc(ip->size - len);
    memcpy(new_buf, old_buf, pos);                    // 复制前段
    memcpy(new_buf + pos, old_buf + pos + len, ip->size - pos - len); // 复制后段
    writei(ip, new_buf, 0, ip->size - len); // 写入新内容
    free(old_buf);
    free(new_buf);
    if(deletei(ip, pos, len) != E_SUCCESS) {
        iput(ip);
        return E_ERROR;
    }
    iput(ip);
    return E_SUCCESS;
}

int cmd_login(int client_id,int auid) {
    if(auid < 0)
        return E_INVALID_USER;
    users[client_id].uid = auid;
    users[client_id].current_dir = root->inum;
    return E_SUCCESS;
}

 uint lookup(inode *dp, char *name,int client_id) {
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
            inode *ip = iget(de.inum);
            if(ip == NULL) {
                return 0;
            }
            if(ip->owner_id!=users[client_id].uid&&users[client_id].uid!=0){ 
                iput(ip);
                return 0;
            }
            iput(ip);
            direntblock=off / BSIZE;
            direntoff = block_off;
            return de.inum;
    }
}
    return 0;  
}
 int dirlink(inode *dp, char *name, uint inum,int client_id) {
    struct dirent de;
    uint off;
    if(dp == NULL || name == NULL || dp->type != T_DIR)
        return -1;
        
    if(lookup(dp, name,client_id) != 0)
        return -1;  

    for(off = 0; off < dp->size; off += sizeof(de)) {
        uint block_no = bmap(dp, off / BSIZE);
        if(block_no == 0)
            return -1;
            
        uchar buf[BSIZE];
        read_block(block_no, buf);    
        memmove(&de, buf + (off % BSIZE), sizeof(de));
        
        if(de.inum == 0)  
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
