#ifndef _FILE_H
#define _FILE_H

#include "dirent.h"
#include "inode.h"
#include "sync/spinlock.h"

typedef size_t loff_t;

#define FILE_OPEN_MODE_READ_ONLY    0b01
#define FILE_OPEN_MODE_WRITE_ONLY   0b10
#define FILE_OPEN_MODE_READ_WRITE   0b11

// Открытый файл
typedef struct PACKED {
    vfs_inode_t*    inode;
    int             mode;
    int             flags;
    loff_t          pos;
    void*           private_data;
    spinlock_t      spinlock;
} file_t;

#endif