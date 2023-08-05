#include "file.h"
#include "mem/kheap.h"
#include "string.h"
#include "vfs.h"

struct file* new_file()
{
    struct file* file = kmalloc(sizeof(struct file));
    memset(file, 0, sizeof(struct file));
    return file;
}

void file_close(struct file* file) 
{
    acquire_spinlock(&file->lock);
    
    if (file->inode) {
        inode_close(file->inode);
    }

    if (file->dentry) {
        dentry_close(file->dentry);
    }

    kfree(file);
}

struct file* file_open(struct dentry* dir, const char* path, int flags, int mode)
{
    struct dentry* dentry;
    struct inode* inode = vfs_fopen(dir, path, flags, &dentry);

    if (!inode) {
        return NULL;
    }

    struct file* file = new_file();
    file->inode = inode;
    file->mode = mode;
    file->flags = flags;
    file->pos = 0;
    file->dentry = dentry;
    file->ops = inode->file_ops;

    if (file->ops->open) {
        file->ops->open(inode, file);
    }

    return file;
}

struct file* file_clone(struct file* original)
{
    struct file* file = new_file();
    file->inode = original->inode;
    file->dentry = original->dentry;

    inode_open(file->inode, 0);
    dentry_open(file->dentry);
    
    return file;
}

ssize_t file_read(struct file* file, size_t size, char* buffer)
{
    ssize_t read = -1;

    acquire_spinlock(&file->lock);

    if (file->flags & FILE_OPEN_FLAG_DIRECTORY) {
        read = ERROR_IS_DIRECTORY;
        goto exit;
    }
    
    if (file->flags & FILE_OPEN_MODE_READ_ONLY) {
        struct inode* inode = file->inode;
        read = inode_read(inode, &file->pos, size, buffer);
    } else {
        read = ERROR_BAD_FD;
    }

exit:
    release_spinlock(&file->lock);
    return read;
}

ssize_t file_write(struct file* file, size_t size, const char* buffer)
{
    ssize_t read = -1;

    acquire_spinlock(&file->lock);

    if (file->flags & FILE_OPEN_FLAG_DIRECTORY) {
        read = ERROR_IS_DIRECTORY;
        goto exit;
    }

    if (file->flags & FILE_OPEN_MODE_WRITE_ONLY) {
        read = file->ops->write(file, buffer, size, file->pos);
    }

exit:
    release_spinlock(&file->lock);
    return 0;
}

off_t file_seek(struct file* file, off_t offset, int whence)
{
    off_t result = -1;

    if ( !(whence >= SEEK_SET && whence <= SEEK_END)) {
        return -ERROR_INVALID_VALUE;
    }

    acquire_spinlock(&file->lock);

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
    release_spinlock(&file->lock);
    return result;
}

int file_readdir(struct file* file, struct dirent* dirent)
{
    int result_code = 0;

    acquire_spinlock(&file->lock);

    struct inode* inode = file->inode;

    if ( !(inode->mode & INODE_TYPE_DIRECTORY) ) {
        // Это не директория
        result_code = ERROR_NOT_A_DIRECTORY;
        goto exit;
    }

    struct dirent* ndirent = inode_readdir(inode, file->pos ++);
        
    release_spinlock(&file->lock);
        
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