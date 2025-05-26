#ifndef __FS_H__
#define __FS_H__

#include "common.h"
#include "inode.h"
#define ROOTINO 2
extern inode *root;
extern inode *cwd;
extern int current_uid;
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
    
} entry;
//
struct dirent {
    uint inum;               // 目录项对应的inode号
    char name[MAXNAME];      // 目录项名称
};//32B
void sbinit();

int cmd_f(int ncyl, int nsec);

int cmd_mk(char *name, short mode);
int cmd_mkdir(char *name, short mode);
int cmd_rm(char *name);
int cmd_rmdir(char *name);

int cmd_cd(char *name);
int cmd_ls(entry **entries, int *n);

int cmd_cat(char *name, uchar **buf, uint *len);
int cmd_w(char *name, uint len, const char *data);
int cmd_i(char *name, uint pos, uint len, const char *data);
int cmd_d(char *name, uint pos, uint len);

int cmd_login(int auid);
uint lookup(inode *dp, char *name);
inode* namei(char *path);
int dirlink(inode *dp, char *name, uint inum);

void fs_shutdown();
#endif