#ifndef __COMMAN_H__
#define __COMMAN_H__

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

// block size in bytes
#define BSIZE 256

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// error codes
enum {
    E_SUCCESS = 0,
    E_ERROR = 1,
    E_NOT_LOGGED_IN = 2,
    E_NOT_FORMATTED = 3,
    E_PERMISSION_DENIED = 4,
    E_IS_DIRECTORY=5,
    E_FILE_NOT_FOUND = 6,
    E_DIR_NOT_FOUND = 7,
    E_NOT_DIRECTORY = 8,
    E_INVALID_NAME = 9,
    E_DIR_NOT_EMPTY = 10,
    E_INVALID_POSITION = 11,
    E_INVALID_RANGE = 12,
    E_INVALID_USER = 13,
    
};

#endif
