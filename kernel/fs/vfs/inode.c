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
    
    if(node)
        if(node->operations->open)
            node->operations->open(node, flags);

    release_spinlock(&node->spinlock);
}

void inode_close(struct inode* node)
{
    acquire_spinlock(&node->spinlock);

    if(node)
        if(node->operations->close)
            node->operations->close(node);

    if (atomic_dec_and_test(&node->reference_count)) {
        superblock_remove_inode(node->sb, node);
        kfree(node);
    }

    release_spinlock(&node->spinlock);
}

void inode_chmod(struct inode* node, uint32_t mode)
{
    acquire_spinlock(&node->spinlock);

    if(node)
        if(node->operations->chmod)
            return node->operations->chmod(node, mode);

    release_spinlock(&node->spinlock);
}

ssize_t inode_read(struct inode* node, uint32_t offset, uint32_t size, char* buffer)
{
    ssize_t result = 0;
    acquire_spinlock(&node->spinlock);

    if(node) {
        if(node->operations->read) {
            result = node->operations->read(node, offset, size, buffer);
        }
    }

    release_spinlock(&node->spinlock);

    return result;
}

struct dirent* inode_readdir(struct inode* node, uint32_t index)
{
    struct dirent* result = NULL;

    acquire_spinlock(&node->spinlock);

    if(node)
        if(node->operations->readdir)
            result = node->operations->readdir(node, index);

    release_spinlock(&node->spinlock);

    return result;
}