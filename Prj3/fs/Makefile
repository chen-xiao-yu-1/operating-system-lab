CC = gcc
CFLAGS = -Wall -Wextra -g

SRCS = main.c inode.c block.c fs.c
OBJS = $(SRCS:.c=.o)

TARGET = fs

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
