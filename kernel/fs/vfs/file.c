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
    if (atomic_dec_and_test(&file->refs)) {

        acquire_spinlock(&file->lock);
        
        if (file->inode) {
            inode_close(file->inode);
        }

        if (file->dentry) {
            dentry_close(file->dentry);
        }

        kfree(file);
    }
}

void split_path(const char* path, char** directory_path_ptr, char** filename_ptr) {
    // Поиск разделителя файла
    char* last_slash = strrchr(path, '/');

    if (last_slash != NULL) {
        // Разделитель найден
        *filename_ptr = last_slash + 1;
        // Длина строки директории
        size_t dir_size = *filename_ptr - path;

        *directory_path_ptr = kmalloc(dir_size);
        strncpy(*directory_path_ptr, path, dir_size - 1);
    } else {
        // Разделителей в пути нет
        *filename_ptr = (char*)path;
        *directory_path_ptr = NULL;
    }
}

int mkfile(struct dentry* dir, const char* path, int mode) {

    int result = 0;
    char* directory_path = NULL;
    char* filename = NULL;

    // Разделить путь на путь директории имя файла
    split_path(path, &directory_path, &filename);

    // Открываем inode директории
    struct inode* dir_inode = vfs_fopen(dir, directory_path, 0, NULL);

    if (dir_inode == NULL) {
        // Отсутствует полный путь к директории
        goto exit;
    }

    // Создать файл
    int rc = inode_mkfile(dir_inode, filename, mode);
    inode_close(dir_inode);
    result = 1;

exit:
    if (directory_path)
        kfree(directory_path);
    
    return result;
}

struct file* file_open(struct dentry* dir, const char* path, int flags, int mode)
{
    struct dentry* dentry;
    struct inode* inode = vfs_fopen(dir, path, flags, &dentry);

    if (!inode) {
        // Файл не найден
        if (flags & FILE_OPEN_FLAG_CREATE) {
            
            // Создаем новый файл
            if (!mkfile(dir, path, mode))
                return NULL;

            // Файл создан, открываем его
            inode = vfs_fopen(dir, path, flags, &dentry);
        } else {

            return NULL;
        }
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

    atomic_inc(&file->refs);

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
        if (file->ops->read) {
            read = file->ops->read(file, buffer, size, file->pos);
        }
    } else {
        read = ERROR_BAD_FD;
    }

exit:
    release_spinlock(&file->lock);
    return read;
}

ssize_t file_write(struct file* file, size_t size, const char* buffer)
{
    ssize_t written = -1;

    acquire_spinlock(&file->lock);

    if (file->flags & FILE_OPEN_FLAG_DIRECTORY) {
        written = ERROR_IS_DIRECTORY;
        goto exit;
    }

    if (file->flags & FILE_OPEN_MODE_WRITE_ONLY) {
        if (file->ops->write) {
            written = file->ops->write(file, buffer, size, file->pos);
        }
    } else {
        written = ERROR_BAD_FD;
    }

exit:
    release_spinlock(&file->lock);
    return written;
}

int file_ioctl(struct file* file, uint64_t request, uint64_t arg)
{
    int result = 0;
    acquire_spinlock(&file->lock);
    
    if (file->ops->ioctl) {
        result = file->ops->ioctl(file, request, arg);
    }

    release_spinlock(&file->lock);

    return result;
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

    struct dirent* ndirent = NULL;
    if (file->ops->readdir) {
        ndirent = file->ops->readdir(file, file->pos ++);
    }
        
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