#include "file.h"
#include "mem/kheap.h"
#include "string.h"
#include "vfs.h"

int file_allow_read(struct file* file)
{
    int masked_flags = file->flags & FILE_OPEN_MODE_MASK;
    return masked_flags != FILE_OPEN_MODE_WRITE_ONLY;
}

int file_allow_write(struct file* file)
{
    int masked_flags = file->flags & FILE_OPEN_MODE_MASK;
    return masked_flags != FILE_OPEN_MODE_READ_ONLY;
}

struct file* new_file()
{
    struct file* file = kmalloc(sizeof(struct file));
    if (file == NULL) {
        return NULL;
    }

    memset(file, 0, sizeof(struct file));
    return file;
}

void file_acquire(struct file* file)
{
    atomic_inc(&file->refs);
}

void file_close(struct file* file) 
{
    if (atomic_dec_and_test(&file->refs)) {
        //printk("CLOSING FILE %i - %i\n", file->inode->inode, file->refs);
        
        if (file->ops && file->ops->close) {
            file->ops->close(file->inode, file);
        }

        if (file->inode) {
            inode_close(file->inode);
        }

        if (file->dentry) {
            dentry_close(file->dentry);
        }

        kfree(file);
    }
}

char* format_path(const char* path) 
{
    char *result = strdup(path);
    if (result[strlen(path) - 1] == '/')
        result[strlen(path) - 1] = '\0';

    return result;
}

void split_path(const char* path, char** directory_path_ptr, char** filename_ptr) 
{
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

int mkfile(struct dentry* dir, const char* path, int mode) 
{
    int rc = 0;
    char* directory_path = NULL;
    char* filename = NULL;

    // Разделить путь на путь директории имя файла
    split_path(path, &directory_path, &filename);

    // Открываем inode директории
    struct inode* dir_inode = vfs_fopen(dir, directory_path, NULL);

    if (dir_inode == NULL) 
    {
        // Отсутствует полный путь к директории
        rc = -ENOENT;
        goto exit;
    }

    // Создать файл
    rc = inode_mkfile(dir_inode, filename, mode);
    inode_close(dir_inode);

exit:
    if (directory_path)
        kfree(directory_path);
    
    return rc;
}

struct file* file_open(struct dentry* dir, const char* path, int flags, int mode)
{
    struct file* result = NULL;
    file_open_ex(dir, path, flags, mode, &result);
    return result;
}

int file_open_ex(struct dentry* dir, const char* path, int flags, int mode, struct file** pfile)
{
    struct dentry* dentry;
    struct inode* inode = vfs_fopen_ex(dir, path, &dentry, flags);
    int rc = 0;

    if (!inode) {
        // Файл не найден
        if (flags & FILE_OPEN_FLAG_CREATE) 
        {
            // Создаем новый файл
            rc = mkfile(dir, path, mode);
            if (rc != 0)
            {
                return rc;
            }

            // Файл создан, открываем его
            inode = vfs_fopen_ex(dir, path, &dentry, flags);
            if (inode == NULL) 
            {
                return -ERROR_NO_FILE;
            }

        } else 
        {
            return -ERROR_NO_FILE;
        }
    } else if ((flags & (O_EXCL| O_CREAT)) == (O_EXCL | O_CREAT)) 
    {
        DENTRY_CLOSE_SAFE(dentry)
        INODE_CLOSE_SAFE(inode)
        return -EEXIST;
    }

    struct file* file = new_file();
    file->inode = inode;
    file->flags = flags;
    file->pos = 0;
    file->dentry = dentry;
    file->ops = inode->file_ops;

    if (file->ops != NULL && file->ops->open) 
    {
        file->ops->open(inode, file);
    }

    *pfile = file;
    
    return 0;
}

ssize_t file_read(struct file* file, size_t size, char* buffer)
{
    ssize_t read = -1;

    acquire_spinlock(&file->lock);

    if (file->flags & FILE_OPEN_FLAG_DIRECTORY) {
        read = -ERROR_IS_DIRECTORY;
        goto exit;
    }
    
    if (file_allow_read(file)) {
        if (file->ops && file->ops->read) {
            read = file->ops->read(file, buffer, size, file->pos);
        } else {
            read = -ERROR_INVALID_VALUE;
        }
    } else {
        read = -ERROR_BAD_FD;
    }

exit:
    release_spinlock(&file->lock);
    return read;
}

ssize_t file_write(struct file* file, size_t size, const char* buffer)
{
    ssize_t written = -1;

    //acquire_spinlock(&file->lock);

    if (file->flags & FILE_OPEN_FLAG_DIRECTORY) {
        written = -ERROR_IS_DIRECTORY;
        goto exit;
    }

    if (file_allow_write(file)) {
        if (file->ops->write) {
            if ((file->flags & FILE_OPEN_FLAG_APPEND) == FILE_OPEN_FLAG_APPEND) {
                file->pos = file->inode->size;
            }

            if (file->ops && file->ops->write) {
                written = file->ops->write(file, buffer, size, file->pos);
            } else {
                written = -ERROR_INVALID_VALUE;
            }
        }
    } else {
        written = -ERROR_BAD_FD;
    }

exit:
    //release_spinlock(&file->lock);
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

    if ((inode->mode & INODE_TYPE_MASK) != INODE_TYPE_DIRECTORY ) 
    {
        // Это не директория
        result_code = -ERROR_NOT_A_DIRECTORY;
        goto exit;
    }

    struct dirent* ndirent = NULL;
    if (file->ops && file->ops->readdir) {
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