#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>

#define MAX_CMD_LEN 1024

int main(int argc, char *argv[]) {
    int c, n, delay;
    char *filename;
    int b = 256;

    if (argc != 6) {
        fprintf(stderr, "Usage: %s <#cylinders> <#sectors> <delay> <diskfile> <DiskPort>\n", argv[0]);
        exit(1);
    }

    c = atoi(argv[1]);
    n = atoi(argv[2]);
    delay = atoi(argv[3]);
    filename = argv[4];
    int port = atoi(argv[5]);

    FILE *log_fp = fopen("disk.log", "w");
    if (!log_fp) {
        perror("Cannot open log file");
        exit(1);
    }

    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("Error opening disk file");
        exit(1);
    }

    long FILESIZE = b * n * c;
    if (lseek(fd, FILESIZE - 1, SEEK_SET) == -1 || write(fd, "", 1) != 1) {
        perror("Error initializing disk file");
        close(fd);
        exit(1);
    }

    char *diskfile = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (diskfile == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 || listen(sockfd, 1) < 0) {
        perror("Socket setup failed");
        exit(1);
    }

    printf("Disk server listening on port %d...\n", port);
    int connfd = accept(sockfd, NULL, NULL);
    if (connfd < 0) {
        perror("Accept failed");
        exit(1);
    }

    char command[MAX_CMD_LEN];
    int last_track = 0;

    while (1) {
        memset(command, 0, sizeof(command));
        int len = recv(connfd, command, sizeof(command) - 1, 0);
        if (len <= 0) break;
        command[strcspn(command, "\n")] = 0;

        char *op = strtok(command, " ");
        if (!op) continue;

        switch (op[0]) {
            case 'I': {
                char buf[64];
                snprintf(buf, sizeof(buf), "%d %d\n", c, n);
                send(connfd, buf, strlen(buf), 0);
                fprintf(log_fp, "%d %d\n", c, n);
                fflush(log_fp);
                break;
            }
            case 'R': {
                int track = atoi(strtok(NULL, " "));
                int sector = atoi(strtok(NULL, " "));
                if (track < 0 || track >= c || sector < 0 || sector >= n) {
                    send(connfd, "NO\n", 3, 0);
                    fprintf(log_fp, "NO\n");
                    fflush(log_fp);
                    break;
                }

                if (last_track != -1 && last_track != track) {
                    usleep(delay * 1000*abs(track - last_track)); 
                }
                last_track = track;

                long offset = (track * n + sector) * b;
                char buffer[b];
                memcpy(buffer, diskfile + offset, b);
                send(connfd, "YES\n", 4, 0);
                send(connfd, buffer, b, 0);  
                fprintf(log_fp, "YES\n");
                break;
            }
            case 'W': {
                int track = atoi(strtok(NULL, " "));
                int sector = atoi(strtok(NULL, " "));
                char *data = strtok(NULL, "");
                if (track < 0 || track >= c || sector < 0 || sector >= n) {
                    send(connfd, "NO\n", 3, 0);
                    fprintf(log_fp, "NO\n");
                    break;
                }

                if (last_track != -1 && last_track != track) {
                    usleep(delay * 1000*abs(track - last_track));
                }
                last_track = track;

                long offset = (track * n + sector) * b;
                char buffer[b];
                memset(buffer, 0, b);
                if (data) memcpy(buffer, data, b);
                memcpy(diskfile + offset, buffer, b);
                send(connfd, "YES\n", 4, 0);
                fprintf(log_fp, "YES\n");
                break;
            }
            case 'E': {
                send(connfd, "Goodbye\n", 8, 0);
                fprintf(log_fp, "Goodbye\n");
                fflush(log_fp);
                fclose(log_fp);
                munmap(diskfile, FILESIZE);
                close(fd);
                close(connfd);
                close(sockfd);
                exit(0);
            }
            default:
                send(connfd, "ERROR\n", 6, 0);
        }
    }

    return 0;
}
