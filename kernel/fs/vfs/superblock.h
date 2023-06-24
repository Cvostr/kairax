#ifndef _SUPERBLOCK_H
#define _SUPERBLOCK_H

#include "types.h"
#include "inode.h"
#include "list.h"

struct super_operations;

typedef struct PACKED {
    list_t*             inodes;
    super_operations    operations;
} superblock_t;

struct super_operations {

    vfs_inode_t *(*alloc_inode)(struct super_block *sb);

    void (*destroy_inode)(vfs_inode_t *);
};

#endif