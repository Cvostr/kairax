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

struct file;

struct file_operations {
    loff_t (*llseek) (struct file* file, loff_t offset, int whence);
    ssize_t (*read) (struct file* file, char* buffer, size_t count, loff_t offset);
    ssize_t (*write) (struct file* file, const char* buffer, size_t count, loff_t offset);
    struct dirent* (*readdir)(struct file* file, uint32_t index);
    int (*ioctl)(struct file* file, uint64_t request, uint64_t arg);

    int (*open) (struct inode *inode, struct file *file);
    int (*release) (struct inode *inode, struct file *file);
};

// Открытый файл
struct file {
    struct inode*           inode;
    struct dentry*          dentry;
    struct file_operations* ops;
    int                     mode;
    int                     flags;
    loff_t                  pos;
    void*                   private_data;
    spinlock_t              lock;
    atomic_t                refs;
};

struct file* new_file();

struct file* file_open(struct dentry* dir, const char* path, int flags, int mode);

struct file* file_clone(struct file* original);

void file_close(struct file* file);

ssize_t file_read(struct file* file, size_t size, char* buffer);

ssize_t file_write(struct file* file, size_t size, const char* buffer);

off_t file_seek(struct file* file, off_t offset, int whence);

int file_readdir(struct file* file, struct dirent* dirent);

#endif