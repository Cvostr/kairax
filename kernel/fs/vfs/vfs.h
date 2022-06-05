#ifndef _VFS_H
#define _VFS_H

#include "stdint.h"
#include "drivers/storage/partitions/storage_partitions.h"


typedef struct PACKED {
    drive_partition_t*   partition;
    char                        mount_path[64];
} vfs_mount_info_t;

void vfs_init();

int vfs_mount(char* mount_path, drive_partition_t* partition);

int vfs_unmount(char* mount_path);

vfs_mount_info_t* vfs_get_mounted_partition(char* mount_path);

vfs_mount_info_t* vfs_get_mounted_partition_split(char* path, int* offset);

vfs_mount_info_t** vfs_get_mounts();

#endif