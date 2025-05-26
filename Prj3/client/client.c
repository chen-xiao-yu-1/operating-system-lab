#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 8192

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <FSPort>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fs_port = atoi(argv[1]);
    if (fs_port <= 0 || fs_port > 65535) {
        fprintf(stderr, "Invalid port number: %d\n", fs_port);
        exit(EXIT_FAILURE);
    }

    const char *fs_ip = "127.0.0.1";  // 默认本地回环

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(fs_port);

    if (inet_pton(AF_INET, fs_ip, &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to FS server on port %d\n", fs_port);
    printf("Type commands; 'e' &' ' to quit.\n");

    char sendbuf[BUF_SIZE];
    char recvbuf[BUF_SIZE];

    while (1) {
    printf("> ");
    if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL) {
        printf("\nEOF, exiting.\n");
        break;
    }

    if (send(sockfd, sendbuf, strlen(sendbuf), 0) <= 0) {
        perror("send");
        break;
    }

    ssize_t rlen = recv(sockfd, recvbuf, sizeof(recvbuf) - 1, 0);
    if (rlen <= 0) {
        perror("recv");
        break;
    }
    recvbuf[rlen] = '\0';
    printf("%s", recvbuf);
}


    close(sockfd);
    return 0;
}
