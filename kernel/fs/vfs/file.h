#ifndef _FILE_H
#define _FILE_H

#include "dirent.h"
#include "inode.h"
#include "sync/spinlock.h"
#include "dentry.h"
#include "errors.h"
#include "mem/vm_area.h"

#define FILE_OPEN_MODE_READ_ONLY    00000000
#define FILE_OPEN_MODE_WRITE_ONLY   00000001
#define FILE_OPEN_MODE_READ_WRITE   00000002
#define FILE_OPEN_MODE_MASK         0b11

#define FILE_OPEN_FLAG_CREATE       0100 // O_CREAT
#define O_CREAT                     0100
#define O_EXCL		                0200
#define O_NOCTTY	                0400
#define FILE_OPEN_FLAG_TRUNCATE     00001000
#define FILE_OPEN_FLAG_APPEND       00002000
#define FILE_OPEN_FLAG_DIRECTORY    00200000
#define FILE_FLAG_NONBLOCK	        00004000 // O_NONBLOCK
#define O_NOFOLLOW	                 0400000
#define O_CLOEXEC                   02000000
#define O_PATH		                010000000

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define FD_CWD		-2
#define AT_EMPTY_PATH  0x1000
#define AT_SYMLINK_NOFOLLOW	0x100

struct file;

struct file_operations {
    loff_t (*llseek) (struct file* file, loff_t offset, int whence);
    ssize_t (*read) (struct file* file, char* buffer, size_t count, loff_t offset);
    ssize_t (*write) (struct file* file, const char* buffer, size_t count, loff_t offset);
    struct dirent* (*readdir)(struct file* file, uint32_t index);
    int (*ioctl)(struct file* file, uint64_t request, uint64_t arg);
    int (*mmap)(struct file* file, struct mmap_range *area);

    int (*open) (struct inode *inode, struct file *file);
    int (*close) (struct inode *inode, struct file *file);
};

// Открытый файл
struct file {
    struct inode*           inode;
    struct dentry*          dentry;
    struct file_operations* ops;
    int                     flags;
    loff_t                  pos;
    void*                   private_data;
    spinlock_t              lock;
    atomic_t                refs;
};

int file_allow_read(struct file* file);

int file_allow_write(struct file* file);

struct file* new_file();

struct file* file_open(struct dentry* dir, const char* path, int flags, int mode);
int file_open_ex(struct dentry* dir, const char* path, int flags, int mode, struct file** pfile);

void file_acquire(struct file* file);
void file_close(struct file* file);

ssize_t file_read(struct file* file, size_t size, char* buffer);

ssize_t file_write(struct file* file, size_t size, const char* buffer);

int file_ioctl(struct file* file, uint64_t request, uint64_t arg);

off_t file_seek(struct file* file, off_t offset, int whence);

int file_readdir(struct file* file, struct dirent* dirent);

char* format_path(const char* path);

// Делит путь на имя файла и директории
// Указатель filename_ptr берется из path
void split_path(const char* path, char** directory_path_ptr, char** filename_ptr);

#endif