#ifndef _TMPFS_H
#define _TMPFS_H

#include "types.h"
#include "../vfs/vfs.h"
#include "../vfs/file.h"
#include "../vfs/superblock.h"

struct tmpfs_dentry {
    uint64_t    inode;
    char*       name;
};

struct tmpfs_inode {
    uint16_t    mode;
    uid_t       uid;
    gid_t       gid;
    size_t      size;

    // Для файловых inode
    uint8_t             *bytes; 
    // Для директорий
    struct tmpfs_dentry *dentries;
};

struct tmpfs_instance {
    size_t blocksize;
    size_t inodes_num;

    struct superblock  *vfs_sb;
    struct tmpfs_inode *root;

    size_t inodes_allocated;
    struct tmpfs_inode **inodes;
};

void tmpfs_init();
void tmpfs_instance_add_inode(struct tmpfs_instance *inst, struct tmpfs_inode* inode, uint32_t index);
struct inode* tmpfs_inode_to_vfs_inode(struct tmpfs_instance *inst, struct tmpfs_inode* inode, uint32_t ino_num);

struct inode* tmpfs_mount(drive_partition_t* drive, struct superblock* sb);

struct inode* tmpfs_read_node(struct superblock* sb, uint64_t ino_num);

#endif