#ifndef _DEVFS_H
#define _DEVFS_H

#include "types.h"
#include "../vfs/vfs.h"
#include "../vfs/file.h"
#include "../vfs/superblock.h"

struct devfs_device {
    struct inode* inode;
    struct dentry* dentry;
};

void devfs_init();

struct devfs_device* new_devfs_device_struct();

struct inode* devfs_mount(drive_partition_t* drive, struct superblock* sb);

void devfs_open(struct inode* inode, uint32_t flags);

struct dirent* devfs_readdir(struct inode* dir, uint32_t index);

struct inode* devfs_read_node(struct superblock* sb, uint64_t ino_num);

uint64 devfs_find_dentry(struct superblock* sb, uint64_t parent_inode_index, const char *name);

int devfs_add_char_device(const char* name, struct file_operations* fops);

#endif