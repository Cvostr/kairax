#include "inode.h"
#include "dirent.h"
#include "mem/kheap.h"
#include "string.h"

struct inode* new_vfs_inode()
{
    struct inode* result = kmalloc(sizeof(struct inode));
    memset(result, 0, sizeof(struct inode));
    return result;
}

void vfs_open(struct inode* node, uint32_t flags)
{
    if(node)
        if(node->operations->open)
            node->operations->open(node, flags);
}

void vfs_chmod(struct inode* node, uint32_t mode)
{
    if(node)
        if(node->operations->chmod)
            return node->operations->chmod(node, mode);
}

ssize_t vfs_read(struct inode* file, uint32_t offset, uint32_t size, char* buffer)
{
    ssize_t result = 0;
    acquire_spinlock(&file->spinlock);

    if(file) {
        if(file->operations->read) {
            result = file->operations->read(file, offset, size, buffer);
        }
    }

    release_spinlock(&file->spinlock);

    return result;
}

struct inode* vfs_finddir(struct inode* node, char* name)
{
    if(node)
        if(node->operations->finddir)
            return node->operations->finddir(node, name);
}

struct dirent* vfs_readdir(struct inode* node, uint32_t index)
{
    if(node)
        if(node->operations->readdir)
            return node->operations->readdir(node, index);
}

void vfs_close(struct inode* node)
{
    if(node)
        if(node->operations->close)
            node->operations->close(node);

    if (atomic_dec_and_test(&node->reference_count)) {
        kfree(node);
    }
}