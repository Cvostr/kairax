#ifndef _VFS_H
#define _VFS_H

#include "types.h"
#include "drivers/storage/partitions/storage_partitions.h"
#include "filesystems.h"

struct inode* new_vfs_inode();

struct dirent* new_vfs_dirent();

void vfs_init();

int vfs_mount(char* mount_path, drive_partition_t* partition);

int vfs_mount_fs(char* mount_path, drive_partition_t* partition, char* fsname);

int vfs_unmount(char* mount_path);

struct superblock* vfs_get_mounted_partition(const char* mount_path);

struct superblock* vfs_get_mounted_partition_split(const char* path, int* offset);

struct superblock** vfs_get_mounts();

//Функции файловой системы

struct inode* vfs_fopen(const char* path, uint32_t flags, struct dentry** dentry);

#endif