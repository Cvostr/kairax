#include "inode.h"
#include "dirent.h"
#include "mem/kheap.h"

ssize_t vfs_read(struct inode* file, uint32_t offset, uint32_t size, char* buffer)
{
    if(file)
        if(file->operations.read)
            return file->operations.read(file, offset, size, buffer);
}

struct inode* vfs_finddir(struct inode* node, char* name)
{
    if(node)
        if(node->operations.finddir)
            return node->operations.finddir(node, name);
}

struct dirent* vfs_readdir(struct inode* node, uint32_t index)
{
    if(node)
        if(node->operations.readdir)
            return node->operations.readdir(node, index);
}

void vfs_close(struct inode* node)
{
    if(node)
        if(node->operations.close)
            node->operations.close(node);

    if (atomic_dec_and_test(&node->reference_count)) {
        vfs_unhold_inode(node);
        kfree(node);
    }
}