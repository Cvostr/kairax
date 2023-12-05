#include "inode.h"
#include "dirent.h"
#include "mem/kheap.h"
#include "string.h"
#include "superblock.h"

struct inode* new_vfs_inode()
{
    struct inode* result = kmalloc(sizeof(struct inode));
    memset(result, 0, sizeof(struct inode));
    return result;
}

void inode_open(struct inode* node, uint32_t flags)
{
    acquire_spinlock(&node->spinlock);

    if (atomic_inc_and_test(&node->reference_count) == 1) {
        superblock_add_inode(node->sb, node);
    }

    release_spinlock(&node->spinlock);
}

void inode_close(struct inode* node)
{
    acquire_spinlock(&node->spinlock);

    if (atomic_dec_and_test(&node->reference_count)) {
        superblock_remove_inode(node->sb, node);
        kfree(node);
    } else {
        release_spinlock(&node->spinlock);
    }
}

int inode_chmod(struct inode* node, uint32_t mode)
{
    int rc = -1;
    acquire_spinlock(&node->spinlock);

    if(node->operations->chmod) {
        rc = node->operations->chmod(node, mode);
        node->mode = (node->mode & 0xFFFFF000) | mode;
    }

    release_spinlock(&node->spinlock);
    return rc;
}

int inode_mkdir(struct inode* node, const char* name, uint32_t mode)
{
    int rc = -1;
    acquire_spinlock(&node->spinlock);

    if(node->operations->mkdir) {
        rc = node->operations->mkdir(node, name, mode);
    }

    release_spinlock(&node->spinlock);

    return rc;
}

int inode_mkfile(struct inode* node, const char* name, uint32_t mode)
{
    acquire_spinlock(&node->spinlock);

    if(node->operations->mkfile) {
        node->operations->mkfile(node, name, mode);
    }

    release_spinlock(&node->spinlock);
}

int inode_truncate(struct inode* inode)
{
    acquire_spinlock(&inode->spinlock);

    int rc = -ERROR_INVALID_VALUE;
    if(inode->operations->truncate) {
        rc = inode->operations->truncate(inode);
    }

    if (rc == 0) {
        inode->size = 0;
    }

    release_spinlock(&inode->spinlock);

    return rc;
}

int inode_unlink(struct inode* parent, struct dentry* child)
{
    acquire_spinlock(&parent->spinlock);

    int rc = -ERROR_INVALID_VALUE;
    if(parent->operations->unlink) {
        rc = parent->operations->unlink(parent, child);
    }

    release_spinlock(&parent->spinlock);

    return rc;
}

int inode_rename(struct inode* parent, struct dentry* orig, struct inode* new_parent, const char* name)
{
    acquire_spinlock(&parent->spinlock);
    acquire_spinlock(&new_parent->spinlock);
    
    int rc = -ERROR_INVALID_VALUE;
    if (parent->operations->rename) {
        rc = parent->operations->rename(parent, orig, new_parent, name);
    }

    release_spinlock(&parent->spinlock);
    release_spinlock(&new_parent->spinlock);

    return rc;
}

int inode_stat(struct inode* node, struct stat* sstat)
{
    acquire_spinlock(&node->spinlock);

    sstat->st_ino = node->inode;
    sstat->st_size = node->size;
    sstat->st_mode = node->mode;
    sstat->st_uid = node->uid;
    sstat->st_gid = node->gid;
    sstat->st_dev = node->device;
    sstat->st_atime = node->access_time;
    sstat->st_ctime = node->create_time;
    sstat->st_mtime = node->modify_time;

    release_spinlock(&node->spinlock);
    return 0;
}