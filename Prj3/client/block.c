#include "block.h"
#include "common.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"


superblock sb;
extern int sockfd;
extern int NCYL, NSEC;
int get_disk_info(int *ncyl, int *nsec) {
    char buf[128];
    char cmd = 'I';
    if (send(sockfd, &cmd, 1, 0) != 1) {
        perror("send disk info request");
        return -1;
    }

    int len = recv(sockfd, buf, sizeof(buf)-1, 0);
    if (len <= 0) {
        perror("recv disk info");
        return -1;
    }
    buf[len] = 0;

    if (sscanf(buf, "%d %d", ncyl, nsec) != 2) {
        fprintf(stderr, "Invalid disk info format: %s\n", buf);
        return -1;
    }
    return 0;
}


void read_block(int blockno, uchar *buf) {
    char msg[64];
    sprintf(msg, "R %d %d\n", blockno / NSEC, blockno % NSEC);
    send(sockfd, msg, strlen(msg), 0);

    char resp[8];
    int i = 0;
    while (i < sizeof(resp) - 1) {
        int len = recv(sockfd, &resp[i], 1, 0);
        if (len <= 0) {
            Error("read_block: failed to receive response");
            exit(1);
        }
        if (resp[i] == '\n') {
            resp[i+1] = '\0';
            break;
        }
        i++;
    }
    if (strncmp(resp, "YES", 3) != 0) {
        Error("read_block: NO from server");
        exit(1);
    }

    int total = 0;
    while (total < BSIZE) {
        int len = recv(sockfd, buf + total, BSIZE - total, 0);
        if (len <= 0) {
            Error("read_block: data recv failed");
            exit(1);
        }
        total += len;
    }
}

void write_block(int blockno, uchar *buf) {
    char msg[64 + BSIZE];
    sprintf(msg, "W %d %d ", blockno / NSEC, blockno % NSEC);
    int offset = strlen(msg);
    memcpy(msg + offset, buf, BSIZE);
    send(sockfd, msg, offset + BSIZE, 0);
    Log("Sent W %d %d", blockno / NSEC, blockno % NSEC);

    char resp[8] = {0};
    int r = recv(sockfd, resp, sizeof(resp), 0);
    Log("Received resp: '%s' (len=%d)", resp, r);

    if (r <= 0) {
        Error("write_block: recv failed or connection closed");
        exit(1);
    }
    if (strncmp(resp, "YES", 3) != 0) {
        Error("write_block: NO from server");
        exit(1);
    }
}


static int is_block_used(uint bno) {
    uchar buf[BSIZE];
    read_block(sb.bmapstart, buf);
    return (buf[bno / 8] & (1 << (bno % 8))) != 0;
}

static void mark_block_used(uint bno) {
    uchar buf[BSIZE];
    read_block(sb.bmapstart, buf);
    buf[bno / 8] |= (1 << (bno % 8));
    write_block(sb.bmapstart, buf);
}

static void mark_block_free(uint bno) {
    uchar buf[BSIZE];
    read_block(sb.bmapstart, buf);
    buf[bno / 8] &= ~(1 << (bno % 8));
    write_block(sb.bmapstart, buf);
}

void zero_block(uint bno) {
    uchar buf[BSIZE];
    memset(buf, 0, BSIZE);
    write_block(bno, buf);
}

uint allocate_block() {
    for (uint b = 2; b < sb.size; b++) {
        if (!is_block_used(b)) {
            mark_block_used(b);
            zero_block(b);
            return b;
        }
    }
    Warn("Out of blocks");
    return 0;
}

void free_block(uint bno) {
    if (bno >= sb.size || bno < 2) {
        Error("free_block: invalid block number");
        return;
    }
    if (!is_block_used(bno)) {
        Error("free_block: block already free");
        return;
    }
    mark_block_free(bno);
    zero_block(bno);
}
