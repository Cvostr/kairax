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
    acquire_spinlock(&file->spinlock);
    
    if (file->inode) {
        inode_close(file->inode);
    }

    if (file->dentry) {
        dentry_close(file->dentry);
    }

    kfree(file);
}

file_t* file_open(struct dentry* dir, const char* path, int flags, int mode)
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

ssize_t file_read(file_t* file, size_t size, char* buffer)
{
    ssize_t read = -1;

    acquire_spinlock(&file->spinlock);

    if (file->flags & FILE_OPEN_FLAG_DIRECTORY) {
        goto exit;
    }
    
    if (file->flags & FILE_OPEN_MODE_READ_ONLY) {
        struct inode* inode = file->inode;
        read = inode_read(inode, &file->pos, size, buffer);
    }

exit:
    release_spinlock(&file->spinlock);
    return read;
}

off_t file_seek(file_t* file, off_t offset, int whence)
{
    off_t result = -1;
    acquire_spinlock(&file->spinlock);

    struct inode* inode = file->inode;

    switch (whence) {
        case SEEK_SET: 
            file->pos = offset;
            break;
        case SEEK_CUR:
            file->pos += offset;
            break;
        case SEEK_END:
            file->pos = inode->size + offset;
            break;
    }

    result = file->pos;

exit:
    release_spinlock(&file->spinlock);
    return result;
}

int file_readdir(file_t* file, struct dirent* dirent)
{
    int result_code = 0;

    acquire_spinlock(&file->spinlock);

    struct inode* inode = file->inode;

    if ( !(inode->mode & INODE_TYPE_DIRECTORY) ) {
        // Это не директория
        result_code = ERROR_NOT_A_DIRECTORY;
        goto exit;
    }

    struct dirent* ndirent = inode_readdir(inode, file->pos ++);
        
    release_spinlock(&file->spinlock);
        
    // Больше в директории ничего нет
    if (ndirent == NULL) {
        result_code = 0;
        goto exit;
    }

    // Скопировать считанный dirent
    memcpy(dirent, ndirent, sizeof(struct dirent));
    kfree(ndirent);
        
    result_code = 1;

exit:
    return result_code;
}