#ifndef _VFS_H
#define _VFS_H

#include "types.h"
#include "drivers/storage/partitions/storage_partitions.h"
#include "filesystems.h"

#define MOUNT_PATH_MAX_LEN 256

typedef struct PACKED {
    drive_partition_t*          partition;
    char                        mount_path[MOUNT_PATH_MAX_LEN];
    filesystem_t*               filesystem;
    struct inode*               root_node;
} vfs_mount_info_t;

struct inode* new_vfs_inode();

struct dirent* new_vfs_dirent();

void vfs_init();

int vfs_mount(char* mount_path, drive_partition_t* partition);

int vfs_mount_fs(char* mount_path, drive_partition_t* partition, char* fsname);

int vfs_unmount(char* mount_path);

vfs_mount_info_t* vfs_get_mounted_partition(const char* mount_path);

vfs_mount_info_t* vfs_get_mounted_partition_split(const char* path, int* offset);

vfs_mount_info_t** vfs_get_mounts();

//Функции файловой системы

ssize_t vfs_read(struct inode* file, uint32_t offset, uint32_t size, char* buffer);

struct inode* vfs_finddir(struct inode* node, char* name);

struct dirent* vfs_readdir(struct inode* node, uint32_t index);

void vfs_chmod(struct inode* node, uint32_t mode);

void vfs_open(struct inode* node, uint32_t flags);

void vfs_close(struct inode* node);

struct inode* vfs_fopen(const char* path, uint32_t flags);

// Функции holder

void init_vfs_holder();

struct inode* vfs_get_inode_by_path(const char* path);

void vfs_hold_inode(struct inode* inode);

void vfs_unhold_inode(struct inode* inode);

#endif