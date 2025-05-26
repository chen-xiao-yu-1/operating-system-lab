#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "block.h"
#include "common.h"
#include "inode.h"
#include "fs.h"
#include "log.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

int sockfd;  
int NCYL, NSEC; 
pthread_mutex_t fs_lock ; 
pthread_mutex_t client_count_lock;

int active_clients = 0;
#define ReplyYes()       \
    do {                 \
        printf("Yes\n"); \
        Log("Success");  \
    } while (0)
#define ReplyNo(x)      \
    do {                \
        printf("No\n"); \
        Warn(x);        \
    } while (0)


void sb_load() {
    uchar buf[BSIZE];
    read_block(0, buf);         // 从第1块读取 superblock
    memcpy(&sb, buf, sizeof(sb));
}

int handle_f(int client_id, char *args, char *response, size_t resp_size) {
    if (cmd_f(NCYL, NSEC,client_id) == E_SUCCESS) {
        snprintf(response, resp_size, "Done\n");
    } else {
        snprintf(response, resp_size, "No\nFailed to format\n");
    }
    return 0;
}

int handle_mk(int client_id,char *args, char *response, size_t resp_size) {
    char *name = strtok(args, " ");
    short mode = 0700;
    if (name && cmd_mk(client_id,name, mode) == E_SUCCESS) {
        snprintf(response, resp_size, "Yes\n");
    } else {
        snprintf(response, resp_size, "No\nFailed to create file\n");
    }
    return 0;
}

int handle_mkdir(int client_id,char *args, char *response, size_t resp_size) {
    char *name = strtok(args, " ");
    short mode = 0700;
    if (name && cmd_mkdir(client_id,name, mode) == E_SUCCESS) {
        snprintf(response, resp_size, "Yes\n");
    } else {
        snprintf(response, resp_size, "No\nFailed to create directory\n");
    }
    return 0;
}

int handle_rm(int client_id,char *args, char *response, size_t resp_size) {
    char *name = strtok(args, " ");
    if (name && cmd_rm(client_id,name) == E_SUCCESS) {
        snprintf(response, resp_size, "Yes\n");
    } else {
        snprintf(response, resp_size, "No\nFailed to remove file\n");
    }
    return 0;
}

int handle_cd(int client_id,char *args, char *response, size_t resp_size) {
    char *name = strtok(args, " ");
    if (name && cmd_cd(client_id,name) == E_SUCCESS) {
        snprintf(response, resp_size, "Yes\n");
    } else {
        snprintf(response, resp_size, "No\nFailed to change directory\n");
    }
    return 0;
}

int handle_rmdir(int client_id,char *args, char *response, size_t resp_size) {
    char *name = strtok(args, " ");
    if (name && cmd_rmdir(client_id,name) == E_SUCCESS) {
        snprintf(response, resp_size, "Yes\n");
    } else {
        snprintf(response, resp_size, "No\nFailed to remove directory\n");
    }
    return 0;
}

int handle_cat(int client_id,char *args, char *response, size_t resp_size) {
    char *name = strtok(args, " ");
    uchar *buf = NULL;
    uint len;
    if (name && cmd_cat(client_id,name, &buf, &len) == E_SUCCESS) {
        snprintf(response, resp_size, "Yes\n");
        size_t offset = strlen(response);
        if (offset + len < resp_size) {
            memcpy(response + offset, buf, len);
            offset += len;
            response[offset] = '\n';
            response[offset + 1] = '\0';
        } else {
            strncat(response, "File too large to display\n", resp_size - offset - 1);
        }
        free(buf);
    } else {
        snprintf(response, resp_size, "No\nFailed to read file\n");
    }
    return 0;
}

int handle_w(int client_id,char *args, char *response, size_t resp_size) {
    char *name = strtok(args, " ");
    char *len_str = strtok(NULL, " ");
    char *data = strtok(NULL, "");
    if (!name || !len_str || !data) {
        snprintf(response, resp_size, "No\nInvalid arguments for write\n");
        return 0;
    }
    uint len = atoi(len_str);
    if (cmd_w(client_id,name, len, data) == E_SUCCESS) {
        snprintf(response, resp_size, "Yes\n");
    } else {
        snprintf(response, resp_size, "No\nFailed to write file\n");
    }
    return 0;
}

int handle_i(int client_id,char *args, char *response, size_t resp_size) {
    char *name = strtok(args, " ");
    char *pos_str = strtok(NULL, " ");
    char *len_str = strtok(NULL, " ");
    char *data = strtok(NULL, "");
    if (!name || !pos_str || !len_str || !data) {
        snprintf(response, resp_size, "No\nInvalid arguments for insert\n");
        return 0;
    }
    uint pos = atoi(pos_str);
    uint len = atoi(len_str);
    if (cmd_i(client_id,name, pos, len, data) == E_SUCCESS) {
        snprintf(response, resp_size, "Yes\n");
    } else {
        snprintf(response, resp_size, "No\nFailed to insert file\n");
    }
    return 0;
}

int handle_d(int client_id,char *args, char *response, size_t resp_size) {
    char *name = strtok(args, " ");
    char *pos_str = strtok(NULL, " ");
    char *len_str = strtok(NULL, " ");
    if (!name || !pos_str || !len_str) {
        snprintf(response, resp_size, "No\nInvalid arguments for delete\n");
        return 0;
    }
    uint pos = atoi(pos_str);
    uint len = atoi(len_str);
    if (cmd_d(client_id,name, pos, len) == E_SUCCESS) {
        snprintf(response, resp_size, "Yes\n");
    } else {
        snprintf(response, resp_size, "No\nFailed to delete in file\n");
    }
    return 0;
}

int handle_ls(int client_id,char *args, char *response, size_t resp_size) {
    entry *entries = NULL;
    int n = 0;
    if (cmd_ls(client_id,&entries, &n) != E_SUCCESS) {
        snprintf(response, resp_size, "No\nFailed to list files\n");
        return 0;
    }
    snprintf(response, resp_size, "Yes\n");
    size_t offset = strlen(response);

    for (int i = 0; i < n; i++) {
        entry *e = &entries[i];
        const char *type_str = (e->type == T_DIR) ? "DIR" : "FILE";

        char timebuf[64];
        time_t mt = e->mtime;
        struct tm *tm_info = localtime(&mt);
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

        int ret = snprintf(response + offset, resp_size - offset,
            "%-4s %-16s %8u bytes  mode %04o  mtime %s  owner %u\n",
            type_str, e->name, e->size, e->mode, timebuf, e->owner_id);


        if (ret < 0 || ret >= (int)(resp_size - offset)) {
            break; 
        }
        offset += ret;
    }

    free(entries);
    return 0;
}


int handle_login(int client_id,char *args, char *response, size_t resp_size) {
    char *uid_str = strtok(args, " ");
    if (!uid_str) {
        snprintf(response, resp_size, "No\nMissing UID\n");
        return 0;
    }
    int uid = atoi(uid_str);
    if (cmd_login(client_id,uid) == E_SUCCESS) {
        snprintf(response, resp_size, "Yes\n");
    } else {
        snprintf(response, resp_size, "No\nFailed to login\n");
    }
    return 0;
}

int handle_e(int client_id, char *args, char *response, size_t resp_size) {
    snprintf(response, resp_size, "Bye!\n");

    pthread_mutex_lock(&client_count_lock);
    users[client_id].uid = -1;  
    pthread_mutex_unlock(&client_count_lock);

    return -1;  
}

typedef int (*cmd_handler_t)(int client_id, char *args, char *response, size_t resp_size);

static struct {
    const char *name;
    cmd_handler_t handler;
} cmd_table[] = {{"f", handle_f},        {"mk", handle_mk},       {"mkdir", handle_mkdir}, {"rm", handle_rm},
                 {"cd", handle_cd},      {"rmdir", handle_rmdir}, {"ls", handle_ls},       {"cat", handle_cat},
                 {"w", handle_w},        {"i", handle_i},         {"d", handle_d},         {"e", handle_e},
                 {"login", handle_login}};

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))


void *handle_client_thread(void *arg) {
    int clientfd = *(int *)arg;
    free(arg);
    pthread_mutex_lock(&client_count_lock);
    active_clients++;
    pthread_mutex_unlock(&client_count_lock);
    User *user = &users[clientfd];
    user->uid = -1;  
    user->current_dir = root->inum;  
    user->client_id = clientfd;
    printf("Client connected (fd=%d)\n", clientfd);
    uchar buf[4096];
    
    while (1) {
        ssize_t n = recv(clientfd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            printf("Client disconnected or error (fd=%d)\n", clientfd);
            close(clientfd);
            break;
        }

        buf[n] = '\0';
        if (buf[n - 1] == '\n') {
            buf[n - 1] = '\0';
        }

        char response[4096];
        char *p = strtok((char *)buf, " ");
        int ret = 1;
        char *args = strtok(NULL, "");

        pthread_mutex_lock(&fs_lock);  
        for (uint i = 0; i < NCMD; i++) {
            if (p && strcmp(p, cmd_table[i].name) == 0) {
                ret = cmd_table[i].handler(clientfd, args, response, sizeof(response));
                break;
            }
        }
        pthread_mutex_unlock(&fs_lock);  

        if (ret == 1) {
            snprintf(response, sizeof(response), "No\n");
        }

        send(clientfd, response, strlen(response), 0);

        if (ret < 0) {
            close(clientfd);
            break;
        }
    }
    pthread_mutex_lock(&client_count_lock);
    active_clients--;
    if (active_clients == 0) {
    exit(0);  // 所有客户端都断了，退出
    }
    pthread_mutex_unlock(&client_count_lock);

    return NULL;
}
void *fs_server_thread(void *arg) {
    int fs_port = *(int *)arg;
    free(arg);

    int listenfd;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("fs_server socket");
        pthread_exit(NULL);
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(fs_port);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("fs_server bind");
        close(listenfd);
        pthread_exit(NULL);
    }

    if (listen(listenfd, 10) < 0) {
        perror("fs_server listen");
        close(listenfd);
        pthread_exit(NULL);
    }

    printf("FS server listening on port %d\n", fs_port);

    while (1) {
        int clientfd = accept(listenfd, NULL, NULL);
        if (clientfd < 0) {
            perror("fs_server accept");
            continue;
        }

        int *fd_ptr = malloc(sizeof(int));
        *fd_ptr = clientfd;
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client_thread, fd_ptr) != 0) {
            perror("pthread_create for client");
            close(clientfd);
            free(fd_ptr);
        } else {
            pthread_detach(tid);  
        }
    }

    close(listenfd);
    pthread_exit(NULL);
}

FILE *log_file;
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: %s <disk_port> <fs_port>\n", argv[0]);
        exit(1);
    }

    int disk_port = atoi(argv[1]);
    int *fs_port_ptr = malloc(sizeof(int));
    *fs_port_ptr = atoi(argv[2]);
    pthread_mutex_init(&fs_lock, NULL);
    pthread_mutex_init(&client_count_lock, NULL);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in disk_addr;
    memset(&disk_addr, 0, sizeof(disk_addr));
    disk_addr.sin_family = AF_INET;
    disk_addr.sin_port = htons(disk_port);
    if (inet_pton(AF_INET, "127.0.0.1", &disk_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&disk_addr, sizeof(disk_addr)) < 0) {
        perror("connect to disk server");
        exit(1);
    }

    printf("Connected to disk server on port %d\n", disk_port);

    get_disk_info(&NCYL, &NSEC);
    log_file = fopen("fs.log", "w");  
    if (!log_file) {
    perror("fopen log_file");
    exit(1);
}

    sb_load();
    if (sb.magic != 817) {
        cmd_f(NCYL, NSEC,0);
        sbinit();
    } else {
        root = iget(sb.rootinum);
        cwd = iget(root->inum);
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, fs_server_thread, fs_port_ptr) != 0) {
        perror("pthread_create");
        exit(1);
    }

    pthread_join(tid, NULL);

    fs_shutdown();
    log_close();
    close(sockfd);
    return 0;
}
