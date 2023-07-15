#ifndef _SUPERBLOCK_H
#define _SUPERBLOCK_H

#include "types.h"
#include "inode.h"
#include "list/list.h"
#include "filesystems.h"
#include "dentry.h"
#include "drivers/storage/partitions/storage_partitions.h"

#define MOUNT_PATH_MAX_LEN 256
struct super_operations;

struct superblock {
    list_t*                     inodes; // список inodes от этого суперблока
    struct dentry*              root_dir;   // dentry монтирования
    struct super_operations*    operations; // операции
    void*                       fs_info; // Указатель на объект ФС
    size_t                      blocksize;

    drive_partition_t*          partition;  // потом надо бы заменить на block_device
    filesystem_t*               filesystem; // Информация о ФС

    spinlock_t      spinlock;
};

struct super_operations {

    struct inode *(*alloc_inode)(struct superblock *sb);

    void (*destroy_inode)(struct inode * inode);

    struct inode* (*read_inode)(struct superblock *sb, uint64_t ino_num);

    uint64_t (*find_dentry)(struct superblock *sb, uint64_t parent_num, const char* name);
};

struct superblock* new_superblock();

void free_superblock(struct superblock* sb);

struct inode* superblock_get_cached_inode(struct superblock* sb, uint64 inode);

struct inode* superblock_get_inode(struct superblock* sb, uint64 inode);

struct dentry* superblock_get_dentry(struct superblock* sb, struct dentry* parent, const char* name);

void superblock_add_inode(struct superblock* sb, struct inode* inode);

void superblock_remove_inode(struct superblock* sb, struct inode* inode);

#endif