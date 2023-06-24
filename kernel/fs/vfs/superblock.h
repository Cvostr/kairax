#ifndef _SUPERBLOCK_H
#define _SUPERBLOCK_H

#include "types.h"
#include "inode.h"
#include "list/list.h"

struct super_operations;

typedef struct PACKED {
    list_t*             inodes;
    struct super_operations*    operations;
} superblock_t;

typedef struct super_operations {

    vfs_inode_t *(*alloc_inode)( superblock_t *sb);

    void (*destroy_inode)(vfs_inode_t * inode);
};

superblock_t* new_superblock();

#endif