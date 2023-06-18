#ifndef _CHARACTER_DEVICES_H
#define _CHARACTER_DEVICES_H

#include "types.h"
#include "fs/vfs/file.h"

#define CHAR_DEVICE_NAME_MAX_LEN     32

typedef struct PACKED {
    loff_t (*llseek) (file_t* file, loff_t, int);
    size_t (*read) (file_t* file, char* buffer, size_t count, loff_t offset);
    size_t (*write) (file_t* file, const char* buffer, size_t count, loff_t offset);
    int (*ioctl)(file_t* file, uint64_t request, uint64_t arg);

    int (*open) (vfs_inode_t *inode, file_t *file);
    int (*release) (vfs_inode_t *inode, file_t *file);
} file_operations;

typedef struct PACKED {
    char                name[CHAR_DEVICE_NAME_MAX_LEN];
    file_operations*    operations_ptr;
} character_device_t;

#endif