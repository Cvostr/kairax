#include "inode.h"
#include "dirent.h"

ssize_t vfs_read(vfs_inode_t* file, uint32_t offset, uint32_t size, char* buffer)
{
    if(file)
        if(file->operations.read)
            return file->operations.read(file, offset, size, buffer);
}

vfs_inode_t* vfs_finddir(vfs_inode_t* node, char* name)
{
    if(node)
        if(node->operations.finddir)
            return node->operations.finddir(node, name);
}

struct dirent* vfs_readdir(vfs_inode_t* node, uint32_t index)
{
    if(node)
        if(node->operations.readdir)
            return node->operations.readdir(node, index);
}

void vfs_close(vfs_inode_t* node)
{
    if(node)
        if(node->operations.close)
            node->operations.close(node);

    if (atomic_dec_and_test(&node->reference_count)) {
        vfs_unhold_inode(node);
        kfree(node);
    }
}