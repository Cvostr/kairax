#ifndef _DEVFS_H
#define _DEVFS_H

#include "types.h"
#include "../vfs/vfs.h"
#include "../vfs/file.h"

void devfs_init();

vfs_inode_t* devfs_mount(drive_partition_t* drive);

void devfs_open(vfs_inode_t* inode, uint32_t flags);

vfs_inode_t* devfs_readdir(vfs_inode_t* dir, uint32_t index);

vfs_inode_t* devfs_finddir(vfs_inode_t* parent, char *name);

#endif