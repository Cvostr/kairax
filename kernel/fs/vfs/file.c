#include "file.h"
#include "mem/kheap.h"
#include "string.h"
#include "vfs.h"

file_t* new_file()
{
    file_t* file = kmalloc(sizeof(file_t));
    memset(file, 0, sizeof(file_t));
    return file;
}

void file_close(file_t* file) 
{
    if (file->inode) {
        inode_close(file->inode);
    }

    if (file->dentry) {
        dentry_close(file->dentry);
    }

    kfree(file);
}

file_t* file_open(struct dentry* dir, const char* path, int mode, int flags)
{
    struct dentry* dentry;
    struct inode* inode = vfs_fopen(dir, path, flags, &dentry);

    if (!inode) {
        return NULL;
    }

    file_t* file = new_file();
    file->inode = inode;
    file->mode = mode;
    file->flags = flags;
    file->pos = 0;
    file->dentry = dentry;

    return file;
}

file_t* file_clone(file_t* original)
{
    file_t* file = new_file();
    file->inode = original->inode;
    file->dentry = original->dentry;

    inode_open(file->inode, 0);
    dentry_open(file->dentry);
    
    return file;
}