#include "tmpfs.h"
#include "mem/kheap.h"
#include "string.h"

struct file_operations tmpfs_file_ops;
struct file_operations tmpfs_dir_ops;

struct inode_operations tmpfs_file_inode_ops;
struct inode_operations tmpfs_dir_inode_ops;

struct super_operations tmpfs_sb_ops = {
    .read_inode = tmpfs_read_node
};

void tmpfs_init()
{
    filesystem_t* tmpfs = new_filesystem();
    tmpfs->name = "tmpfs";
    tmpfs->mount = tmpfs_mount;

    filesystem_register(tmpfs);
}

void tmpfs_instance_add_inode(struct tmpfs_instance *inst, struct tmpfs_inode* inode, uint32_t index)
{
    if (index == 0)
    {

    }

    size_t new_size = (index + 1);
    size_t new_size_bytes = new_size * sizeof(struct tmpfs_inode*);
    if (new_size > inst->inodes_allocated)
    {
        struct tmpfs_inode **inodes = kmalloc(new_size_bytes);
        memset(inodes, 0, new_size_bytes);
        memcpy(inodes, inst->inodes, inst->inodes_allocated * sizeof(struct tmpfs_inode*));
        kfree(inst->inodes);
        inst->inodes = inodes;
    }

    inst->inodes[index] = inode;
}

struct inode* tmpfs_inode_to_vfs_inode(struct tmpfs_instance *inst, struct tmpfs_inode* inode, uint32_t ino_num)
{
    struct inode* result = new_vfs_inode();
    result->inode = ino_num;
    result->sb = inst->vfs_sb;
    result->uid = inode->uid;
    result->gid = inode->gid;
    result->size = inode->size;
    //result->blocks = inode->num_blocks;
    result->mode = inode->mode;
    //result->access_time.tv_sec = inode->atime;
    //result->create_time.tv_sec = inode->ctime;
    //result->modify_time.tv_sec = inode->mtime;
    //result->hard_links = inode->hard_links;
    result->device = (dev_t) inst;

    if ((inode->mode & INODE_TYPE_MASK) == INODE_TYPE_FILE) {
        result->operations = &tmpfs_file_inode_ops;
        result->file_ops = &tmpfs_file_ops;
    }

    if ((inode->mode & INODE_TYPE_MASK) == INODE_TYPE_DIRECTORY) {
        result->operations = &tmpfs_dir_inode_ops;
        result->file_ops = &tmpfs_dir_ops;
    }
    if ((inode->mode & INODE_TYPE_MASK) == INODE_FLAG_SYMLINK) {
        //result->operations = &symlink_inode_ops;
    }
    if ((inode->mode & INODE_TYPE_MASK) == INODE_FLAG_BLOCKDEVICE) {
    
    }

    return result;
}

struct inode* tmpfs_mount(drive_partition_t* drive, struct superblock* sb)
{
    struct tmpfs_instance *inst = kmalloc(sizeof(struct tmpfs_instance));
    memset(inst, 0, sizeof(struct tmpfs_instance));
    inst->vfs_sb = sb;
    inst->blocksize = 4096;

    struct tmpfs_inode *root_inode = kmalloc(sizeof(struct tmpfs_inode));
    memset(root_inode, 0, sizeof(struct tmpfs_inode));
    root_inode->uid = 0;
    root_inode->gid = 0;
    root_inode->mode = INODE_TYPE_DIRECTORY | 0777;

    tmpfs_instance_add_inode(inst, root_inode, 2);

    sb->fs_info = inst;
    sb->blocksize = inst->blocksize;
    sb->operations = &tmpfs_sb_ops;

    return tmpfs_inode_to_vfs_inode(inst, root_inode, 2);
}

struct inode* tmpfs_read_node(struct superblock* sb, uint64_t ino_num)
{
    struct tmpfs_instance *inst = (struct tmpfs_instance*) sb->fs_info;
    return tmpfs_inode_to_vfs_inode(inst, inst->inodes[ino_num], ino_num);
}

struct dirent* tmpfs_file_readdir(struct file* dir, uint32_t index)
{
    struct inode* vfs_inode = dir->inode;
    struct tmpfs_instance * inst = (struct tmpfs_instance *) vfs_inode->sb->fs_info;
}

int tmpfs_mkfile(struct inode* parent, const char* file_name, uint32_t mode)
{
}