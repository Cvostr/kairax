#ifndef _TMPFS_H
#define _TMPFS_H

#include "types.h"
#include "../vfs/vfs.h"
#include "../vfs/file.h"
#include "../vfs/superblock.h"

struct tmpfs_dentry {
    uint64_t    inode;
    int         type;
    char        name[];
};

struct tmpfs_inode {
    uint16_t    mode;
    uid_t       uid;
    gid_t       gid;
    size_t      size;
    uint32_t    hard_links;

    atomic_t    refs;

    // Для файловых inode
    uint8_t             *bytes; 
    // Для директорий
    size_t dentries_allocated;
    struct tmpfs_dentry **dentries;
};

struct tmpfs_instance {
    size_t blocksize;
    size_t inodes_num;

    struct superblock  *vfs_sb;
    struct tmpfs_inode *root;

    size_t inodes_allocated;
    struct tmpfs_inode **inodes;
};

void tmpfs_free_inode(struct tmpfs_inode* inode);

void tmpfs_init();
void tmpfs_instance_remove_inode(struct tmpfs_instance *inst, ino_t index);
ino_t tmpfs_instance_add_inode(struct tmpfs_instance *inst, struct tmpfs_inode* inode, ino_t index);
struct inode* tmpfs_inode_to_vfs_inode(struct tmpfs_instance *inst, struct tmpfs_inode* inode, ino_t ino_num);
struct tmpfs_dentry* tmpfs_make_dentry(uint64_t inode, const char *name, int type);
void tmpfs_inode_add_dentry(struct tmpfs_inode *inode, struct tmpfs_dentry *dentry);
void tmpfs_inode_remove_dentry(struct tmpfs_inode *inode, struct tmpfs_dentry *dentry);
struct tmpfs_inode* tmpfs_get_inode(struct tmpfs_instance *inst, ino_t inode);
struct tmpfs_dentry* tmpfs_get_dentry(struct tmpfs_inode *inode, const char *name);

struct inode* tmpfs_mount(drive_partition_t* drive, struct superblock* sb);
int tmpfs_unmount(struct superblock* sb);

struct inode* tmpfs_read_node(struct superblock* sb, uint64_t ino_num);
uint64_t tmpfs_find_dentry(struct superblock* sb, struct inode* vfs_parent_inode, const char *name, int* type);
void tmpfs_purge_inode(struct inode* inode);

int tmpfs_mkdir(struct inode* parent, const char* dir_name, uint32_t mode);
int tmpfs_mkfile(struct inode* parent, const char* file_name, uint32_t mode);
struct dirent* tmpfs_file_readdir(struct file* dir, uint32_t index);

ssize_t tmpfs_file_read(struct file* file, char* buffer, size_t count, loff_t offset);
ssize_t tmpfs_file_write(struct file* file, const char* buffer, size_t count, loff_t offset);
int tmpfs_unlink(struct inode* parent, struct dentry* dent);
int tmpfs_rmdir(struct inode* parent, struct dentry* dentry);

#endif