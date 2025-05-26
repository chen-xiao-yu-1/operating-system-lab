CC = gcc
CFLAGS = -Wall -Wextra -g -fsanitize=address

FS_SRCS = main.c inode.c block.c fs.c
FS_OBJS = $(FS_SRCS:.c=.o)
FS_TARGET = fs

DISK_SRCS = disk.c
DISK_OBJS = $(DISK_SRCS:.c=.o)
DISK_TARGET = disk

CLIENT_SRCS = client.c
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
CLIENT_TARGET = client

all: $(FS_TARGET) $(DISK_TARGET) $(CLIENT_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(FS_TARGET): $(FS_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(DISK_TARGET): $(DISK_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(FS_OBJS) $(FS_TARGET) $(DISK_OBJS) $(DISK_TARGET) $(CLIENT_OBJS) $(CLIENT_TARGET)

.PHONY: all clean
