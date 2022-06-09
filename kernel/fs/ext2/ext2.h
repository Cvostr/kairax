#ifndef _EXT2_H
#define _EXT2_H

#include "stdint.h"
#include "../vfs/vfs.h"
#include "../vfs/file.h"

#define EXT2_DIRECT_BLOCKS 12

typedef struct PACKED {
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t su_blocks;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t superblock_idx;
    uint32_t log2block_size;
    uint32_t log2frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;

    uint32_t mtime;
    uint32_t wtime;

    uint16_t mount_count;
    uint16_t mount_allowed_count;
    uint16_t ext2_magic;
    uint16_t fs_state;
    uint16_t err;
    uint16_t minor;

    uint32_t last_check;
    uint32_t interval;
    uint32_t os_id;
    uint32_t major;

    uint16_t r_userid;
    uint16_t r_groupid;
    //НЕ ДЛЯ EXT2
    uint32_t first_inode;
    uint16_t inode_size;
    uint16_t superblock_group;
    uint32_t optional_feature;
    uint32_t required_feature;
    uint32_t readonly_feature;
    char fs_id[16];
    char vol_name[16];
    char last_mount_path[64];
    uint32_t compression_method;
    uint8_t file_pre_alloc_blocks;
    uint8_t dir_pre_alloc_blocks;
    uint16_t unused1;
    char journal_id[16];
    uint32_t journal_inode;
    uint32_t journal_device;
    uint32_t orphan_head;

    char unused2[1024-236];
} ext2_superblock_t;

typedef struct PACKED {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t num_dirs;
    uint32_t unused1;
    uint32_t unused2;
} ext2_bgd_t;


typedef struct PACKED {
    uint32_t    inode;
    uint16_t    size;
    uint8_t     name_len;
    uint8_t     type;
    char        name[];
} ext2_direntry_t;

typedef struct PACKED {
    uint16_t permission;
    uint16_t userid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t hard_links;
    uint32_t num_sectors;
    uint32_t flags;
    uint32_t os_specific1;
    uint32_t blocks[EXT2_DIRECT_BLOCKS + 3];
    uint32_t generation;
    uint32_t file_acl;
    union {
        uint32_t dir_acl;
        uint32_t size_high;
    };
    uint32_t f_block_addr;
    char os_specific2[12];
} ext2_inode_t;

typedef struct PACKED {
    drive_partition_t*  partition;
    ext2_superblock_t*  superblock;
    ext2_bgd_t*         bgds;

    size_t              block_size;
    uint64_t            total_groups;

    uint64_t            bgds_blocks;
} ext2_instance_t;

void ext2_init();

int ext2_partition_read_block(ext2_instance_t* inst, uint64_t block_start, uint64_t blocks, char* buffer);

vfs_inode_t* ext2_mount(drive_partition_t* drive);

void ext2_inode(ext2_instance_t* inst, ext2_inode_t* inode, uint64_t node_index);

vfs_inode_t* ext2_inode_to_vfs_inode(ext2_inode_t* inode);

void ext2_mkdir(vfs_inode_t* parent, char* dir_name);

void ext2_mkfile(vfs_inode_t* parent, char* file_name);

void ext2_chmod(vfs_inode_t * file, uint32_t mode);

uint32_t ext2_read(vfs_inode_t* file, uint32_t offset, uint32_t size, char* buffer);

uint32_t ext2_write(vfs_inode_t* file, uint32_t offset, uint32_t size, char* buffer);


#endif