#ifndef __FS_H__
#define __FS_H__

#include "common.h"
#include "inode.h"
#define ROOTINO 2
#define MAXUSERS 20

extern inode *root;
extern inode *cwd;
extern uint direntoff;
extern uint direntblock;
// used for cmd_ls
typedef struct {
    short type;
    char name[MAXNAME];
    uint inum;               // inode号
    uint size;              // 文件大小
    ushort mode;            // 访问权限
    uint mtime;             // 最后修改时间
    int owner_id;         // 文件拥有者
    
} entry;
//
struct dirent {
    uint inum;               // 目录项对应的inode号
    char name[MAXNAME];      // 目录项名称
};//32B

typedef struct{
    int client_id;
    int uid;
    int current_dir;
}User;
extern User users[MAXUSERS];
void sbinit();

int cmd_f(int ncyl, int nsec,int client_id);

int cmd_mk(int client_id,char *name, short mode);
int cmd_mkdir(int client_id,char *name, short mode);
int cmd_rm(int client_id,char *name);
int cmd_rmdir(int client_id,char *name);

int cmd_cd(int client_id,char *name);
int cmd_ls(int client_id,entry **entries, int *n);

int cmd_cat(int client_id,char *name, uchar **buf, uint *len);
int cmd_w(int client_id,char *name, uint len, const char *data);
int cmd_i(int client_id,char *name, uint pos, uint len, const char *data);
int cmd_d(int client_id,char *name, uint pos, uint len);

int cmd_login(int client_id,int auid);
uint lookup(inode *dp, char *name,int client_id);
inode* namei(char *path,int client_id);
int check_write_permission(inode *ip, int client_id);
int check_read_permission(inode *ip, int client_id);
int dirlink(inode *dp, char *name, uint inum,int client_id);

void fs_shutdown();
#endif