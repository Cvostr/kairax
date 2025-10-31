#ifndef _SUPERBLOCK_H
#define _SUPERBLOCK_H

#include "kairax/types.h"
#include "inode.h"
#include "kairax/list/list.h"
#include "filesystems.h"
#include "dentry.h"
#include "drivers/storage/partitions/storage_partitions.h"

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

struct statfs {

    blksize_t blocksize;
    
    blkcnt_t blocks;
    blkcnt_t blocks_free;

    inocnt_t inodes;
    inocnt_t inodes_free;
};

struct super_operations {

    struct inode *(*alloc_inode)(struct superblock *sb);

    void (*destroy_inode)(struct inode* inode);

    struct inode* (*read_inode)(struct superblock *sb, uint64_t ino_num);

    uint64_t (*find_dentry)(struct superblock *sb, struct inode* parent_inode, const char* name, int* type);

    int (*stat)(struct superblock *sb, struct statfs*);
};

struct superblock* new_superblock();

void free_superblock(struct superblock* sb);

// Получить inode по номеру из кеша суперблока
// Извне не использовать
struct inode* superblock_get_cached_inode(struct superblock* sb, uint64 inode);

// Считать inode по номеру из суперблока
struct inode* superblock_get_inode(struct superblock* sb, uint64 inode);

struct dentry* superblock_get_dentry(struct superblock* sb, struct dentry* parent, const char* name);

void superblock_add_inode(struct superblock* sb, struct inode* inode);

void superblock_remove_inode(struct superblock* sb, struct inode* inode);

int superblock_stat(struct superblock* sb, struct statfs* stat);

void debug_print_inodes(struct superblock* sb);

#endif