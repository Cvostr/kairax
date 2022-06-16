#ifndef _VFS_H
#define _VFS_H

#include "stdint.h"
#include "drivers/storage/partitions/storage_partitions.h"
#include "filesystems.h"

typedef struct PACKED {
    drive_partition_t*          partition;
    char                        mount_path[128];
    filesystem_t*               filesystem;
    vfs_inode_t*                root_node;
} vfs_mount_info_t;

vfs_inode_t* new_vfs_inode();

dirent_t* new_vfs_dirent();

void vfs_init();

int vfs_mount(char* mount_path, drive_partition_t* partition);

int vfs_unmount(char* mount_path);

vfs_mount_info_t* vfs_get_mounted_partition(char* mount_path);

vfs_mount_info_t* vfs_get_mounted_partition_split(char* path, int* offset);

vfs_mount_info_t** vfs_get_mounts();

//Функции файловой системы

uint32_t vfs_read(vfs_inode_t* file, uint32_t offset, uint32_t size, char* buffer);

vfs_inode_t* vfs_finddir(vfs_inode_t* node, char* name);

vfs_inode_t* vfs_readdir(vfs_inode_t* node, uint32_t index);

void vfs_chmod(vfs_inode_t* node, uint32_t mode);

void vfs_open(vfs_inode_t* node, uint32_t flags);

void vfs_close(vfs_inode_t* node);

vfs_inode_t* vfs_fopen(const char* path, uint32_t flags);

#endif