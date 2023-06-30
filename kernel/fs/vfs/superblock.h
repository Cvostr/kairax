#ifndef _SUPERBLOCK_H
#define _SUPERBLOCK_H

#include "types.h"
#include "inode.h"
#include "list/list.h"
#include "filesystems.h"
#include "drivers/storage/partitions/storage_partitions.h"

#define MOUNT_PATH_MAX_LEN 256
struct super_operations;

struct superblock {
    list_t*                     inodes; // список inodes от этого суперблока
    //struct dentry*              root_dir;   // dentry монтирования
    struct super_operations*    operations; // операции
    void*                       fs_info; // Указатель на объект ФС
    size_t                      blocksize;

    drive_partition_t*          partition;  // потом надо бы заменить на block_device
    filesystem_t*               filesystem; // Информация о ФС

    char                        mount_path[MOUNT_PATH_MAX_LEN];
};

struct super_operations {

    struct inode *(*alloc_inode)(struct superblock *sb);

    void (*destroy_inode)(struct inode * inode);

    struct inode* (*read_inode)(struct superblock *sb, uint64_t ino_num);
};

struct superblock* new_superblock();

void free_superblock(struct superblock* sb);

struct inode* superblock_get_inode(struct superblock* sb, uint64 inode);

#endif