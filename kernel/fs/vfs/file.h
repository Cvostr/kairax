#ifndef _FILE_H
#define _FILE_H

#include "dirent.h"
#include "dentry.h"
#include "inode.h"
#include "sync/spinlock.h"

#define FILE_OPEN_MODE_READ_ONLY    0b01
#define FILE_OPEN_MODE_WRITE_ONLY   0b10
#define FILE_OPEN_MODE_READ_WRITE   0b11

#define ERROR_BAD_FD                9

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// Открытый файл
typedef struct {
    struct inode*   inode;
    struct dentry*  p_dentry;
    int             mode;
    int             flags;
    loff_t          pos;
    void*           private_data;
    spinlock_t      spinlock;
    char            name[MAX_DIRENT_NAME_LEN];
} file_t;

file_t* new_file();

void file_close(file_t* file);

void file_set_name_from_path(file_t* file, const char* path);

#endif