#ifndef _FILE_H
#define _FILE_H

#include "dirent.h"
#include "inode.h"
#include "sync/spinlock.h"
#include "dentry.h"

#define FILE_OPEN_MODE_READ_ONLY    00000001
#define FILE_OPEN_MODE_WRITE_ONLY   00000002
#define FILE_OPEN_MODE_READ_WRITE   00000003

#define FILE_OPEN_FLAG_CREATE       00000100
#define FILE_OPEN_FLAG_TRUNCATE     00001000
#define FILE_OPEN_FLAG_APPEND       00002000
#define FILE_OPEN_FLAG_DIRECTORY    00200000

// TODO: Перенести
#define ERROR_BAD_FD                9
#define ERROR_NO_FILE               2
#define ERROR_IS_DIRECTORY          21
#define ERROR_INVALID_VALUE         22
#define ERROR_NOT_A_DIRECTORY       20        
#define ERROR_TOO_MANY_OPEN_FILES   24

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define FD_CWD		-2

// Открытый файл
typedef struct {
    struct inode*   inode;
    struct dentry*  dentry;
    int             mode;
    int             flags;
    loff_t          pos;
    void*           private_data;
    spinlock_t      spinlock;
} file_t;

file_t* new_file();

file_t* file_open(struct dentry* dir, const char* path, int flags, int mode);

file_t* file_clone(file_t* original);

void file_close(file_t* file);

ssize_t file_read(file_t* file, size_t size, char* buffer);

off_t file_seek(file_t* file, off_t offset, int whence);

int file_readdir(file_t* file, struct dirent* dirent);

#endif