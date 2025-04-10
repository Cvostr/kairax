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

    atomic_inc(&node->reference_count);

    release_spinlock(&node->spinlock);
}

void inode_close(struct inode* node)
{
    //acquire_spinlock(&node->spinlock);

    if (atomic_dec_and_test(&node->reference_count)) 
    {

        if (node->sb != NULL) 
        {
            if (node->hard_links == 0) 
            {
                //printk("Destroying inode %i\n", node->inode);
                node->sb->operations->destroy_inode(node);
            }

            superblock_remove_inode(node->sb, node);
        }
        kfree(node);
    } else {
        //release_spinlock(&node->spinlock);
    }
}

int inode_check_perm(struct inode* ino, uid_t uid, gid_t gid, int ubit, int gbit, int obit)
{
    // root имеет полные права всегда
    if (uid == 0)
    {
        return 1;
    }

    if (ino->uid == uid)
    {
        return (ino->mode & ubit) == ubit;
    }

    if (ino->gid == gid)
    {
        return (ino->mode & gbit) == gbit;
    }

    return (ino->mode & obit) == obit;
}

int inode_chmod(struct inode* node, uint32_t mode)
{
    int rc = -1;
    acquire_spinlock(&node->spinlock);

    if (node->operations && node->operations->chmod) 
    {
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

    if (node->operations && node->operations->mkdir) 
    {
        rc = node->operations->mkdir(node, name, mode);
    }

    release_spinlock(&node->spinlock);

    return rc;
}

int inode_mkfile(struct inode* node, const char* name, uint32_t mode)
{
    int rc = -1;
    acquire_spinlock(&node->spinlock);

    if(node->operations->mkfile) {
        rc = node->operations->mkfile(node, name, mode);
    }

    release_spinlock(&node->spinlock);

    return rc;
}

int inode_truncate(struct inode* inode)
{
    acquire_spinlock(&inode->spinlock);

    int rc = -ERROR_INVALID_VALUE;
    if (inode->operations && inode->operations->truncate) 
    {
        rc = inode->operations->truncate(inode);
    }

    release_spinlock(&inode->spinlock);

    return rc;
}

int inode_unlink(struct inode* parent, struct dentry* child)
{
    acquire_spinlock(&parent->spinlock);

    int rc = -ERROR_INVALID_VALUE;
    if (parent->operations && parent->operations->unlink) {
        rc = parent->operations->unlink(parent, child);
    }

    release_spinlock(&parent->spinlock);

    return rc;
}

int inode_rmdir(struct inode* parent, struct dentry* child)
{
    acquire_spinlock(&parent->spinlock);

    int rc = -ERROR_INVALID_VALUE;
    if (parent->operations && parent->operations->rmdir) {
        rc = parent->operations->rmdir(parent, child);
    }

    release_spinlock(&parent->spinlock);

    return rc;
}

int inode_rename(struct inode* parent, struct dentry* orig, struct inode* new_parent, const char* name)
{
    acquire_spinlock(&parent->spinlock);
    if (parent != new_parent)
        acquire_spinlock(&new_parent->spinlock);
    
    int rc = -ERROR_INVALID_VALUE;
    if (parent->operations && parent->operations->rename) {
        rc = parent->operations->rename(parent, orig, new_parent, name);
    }

    if (parent != new_parent)
        release_spinlock(&new_parent->spinlock);
    release_spinlock(&parent->spinlock);

    return rc;
}

int inode_linkat(struct dentry* src, struct inode* dst, const char* name)
{
    acquire_spinlock(&dst->spinlock);

    int rc = -EPERM;
    if (dst->operations && dst->operations->link) 
    {
        rc = dst->operations->link(src, dst, name);
    }

    release_spinlock(&dst->spinlock);

    return rc;
}

int inode_mknod(struct inode* parent, const char* name, mode_t mode)
{
    mode_t inode_type = mode & INODE_TYPE_MASK;

    if (inode_type != INODE_FLAG_PIPE &&
        inode_type != INODE_FLAG_SOCKET &&
        inode_type != INODE_TYPE_FILE &&
        inode_type != INODE_FLAG_CHARDEVICE &&
        inode_type != INODE_FLAG_BLOCKDEVICE
    ) 
    {
        return -EINVAL;
    }

    acquire_spinlock(&parent->spinlock);

    int rc = -EINVAL;

    if (parent->operations && parent->operations->mknod)
    {
        rc = parent->operations->mknod(parent, name, mode);
    }

    release_spinlock(&parent->spinlock);

    return rc;
}

int inode_symlink(struct inode* parent, const char* name, const char* target)
{
    if (parent == NULL)
    {
        return -ERROR_INVALID_VALUE;
    }

    acquire_spinlock(&parent->spinlock);

    int rc = -EPERM;
    if (parent->operations && parent->operations->symlink) 
    {
        rc = parent->operations->symlink(parent, name, target);
    }

    release_spinlock(&parent->spinlock);
    
    return rc;
}

ssize_t inode_readlink(struct inode* symlink, char* buf, size_t buflen)
{
    acquire_spinlock(&symlink->spinlock);

    int rc = -EPERM;
    if (symlink->operations && symlink->operations->readlink) 
    {
        rc = symlink->operations->readlink(symlink, buf, buflen);
    }

    release_spinlock(&symlink->spinlock);
    
    return rc;
}

int inode_stat(struct inode* node, struct stat* sstat)
{
    if (node == NULL)
    {
        return -ERROR_INVALID_VALUE;
    }

    acquire_spinlock(&node->spinlock);

    sstat->st_ino = node->inode;
    sstat->st_size = node->size;
    sstat->st_blocks = node->blocks;
    sstat->st_mode = node->mode;
    sstat->st_nlink = node->hard_links;
    sstat->st_uid = node->uid;
    sstat->st_gid = node->gid;
    sstat->st_dev = node->device;
    sstat->st_atime = node->access_time;
    sstat->st_ctime = node->create_time;
    sstat->st_mtime = node->modify_time;

    release_spinlock(&node->spinlock);
    return 0;
}