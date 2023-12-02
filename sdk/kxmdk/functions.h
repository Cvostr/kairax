#ifndef KXMDK_FUNCTIONS_H
#define KXMDK_FUNCTIONS_H

#include "types.h"

int printk(const char* __restrict, ...);

struct file_operations {
    off_t (*llseek) (struct file* file, off_t offset, int whence);
    ssize_t (*read) (struct file* file, char* buffer, size_t count, loff_t offset);
    ssize_t (*write) (struct file* file, const char* buffer, size_t count, loff_t offset);
    struct dirent* (*readdir)(struct file* file, uint32_t index);
    int (*ioctl)(struct file* file, uint64_t request, uint64_t arg);

    int (*open) (struct inode *inode, struct file *file);
    int (*close) (struct inode *inode, struct file *file);
};

int devfs_add_char_device(const char* name, struct file_operations* fops);

#endif