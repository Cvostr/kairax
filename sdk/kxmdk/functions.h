#ifndef KXMDK_FUNCTIONS_H
#define KXMDK_FUNCTIONS_H

#include "mem/vmm.h"
#include "fs/vfs/inode.h"

#define PAGE_SIZE 4096

int printk(const char* __restrict, ...);

void* kmalloc(uint64_t size);
void kfree(void* mem);

void* pmm_alloc_pages(uint32_t pages);
void pmm_free_pages(void* addr, uint32_t pages);

struct file {
    struct inode*           inode;
};

struct file_operations {
    off_t (*llseek) (struct file* file, off_t offset, int whence);
    ssize_t (*read) (struct file* file, char* buffer, size_t count, loff_t offset);
    ssize_t (*write) (struct file* file, const char* buffer, size_t count, loff_t offset);
    struct dirent* (*readdir)(struct file* file, uint32_t index);
    int (*ioctl)(struct file* file, uint64_t request, uint64_t arg);

    int (*open) (struct inode *inode, struct file *file);
    int (*close) (struct inode *inode, struct file *file);
};

int devfs_add_char_device(const char* name, struct file_operations* fops, void* private_data);

#endif