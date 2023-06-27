#ifndef _DEVFS_H
#define _DEVFS_H

#include "types.h"
#include "../vfs/vfs.h"
#include "../vfs/file.h"

void devfs_init();

struct inode* devfs_mount(drive_partition_t* drive);

void devfs_open(struct inode* inode, uint32_t flags);

struct dirent* devfs_readdir(struct inode* dir, uint32_t index);

struct inode* devfs_finddir(struct inode* parent, char *name);

#endif