#ifndef _VFS_H
#define _VFS_H

#include "types.h"
#include "drivers/storage/partitions/storage_partitions.h"
#include "filesystems.h"

struct inode* new_vfs_inode();

struct dirent* new_vfs_dirent();

struct dentry* get_mount_dentry(const char* mount_path);

void vfs_init();

int vfs_mount_fs(const char* mount_path, drive_partition_t* partition, const char* fsname);

int vfs_unmount(char* mount_path);

struct superblock* vfs_get_mounted_partition(const char* mount_path);

struct superblock** vfs_get_mounts();

//Функции файловой системы

struct inode* vfs_fopen(struct dentry* parent, const char* path, struct dentry** dentry);

struct inode* vfs_fopen_parent(struct dentry* child);

struct dentry* vfs_get_root_dentry();

struct dentry* vfs_dentry_traverse_path(struct dentry* parent, const char* path);

void vfs_dentry_get_absolute_path(struct dentry* p_dentry, size_t* p_required_size, char* p_result);

int vfs_is_path_absolute(const char* path);

#endif