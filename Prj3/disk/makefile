CC = gcc
CFLAGS = -Wall -O2
TARGET = disk
SRC = disk.c
LOG = disk.log

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) $(LOG)
