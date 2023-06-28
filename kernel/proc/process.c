#include "process.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "fs/vfs/vfs.h"
#include "string.h"

int process_open_file(process_t* process, const char* path, int mode, int flags)
{
    int fd = -1;
    struct inode* inode = vfs_fopen(path, 0);

    // Создать новый дескриптор файла
    file_t* file = kmalloc(sizeof(file_t));
    file->inode = inode;
    file->mode = mode;
    file->flags = flags;
    file->pos = 0;

    // Найти свободный номер дескриптора для процесса
    acquire_spinlock(&process->fd_spinlock);
    for (int i = 0; i < MAX_DESCRIPTORS; i ++) {
        if (process->fds[i] == NULL) {
            process->fds[i] = file;
            fd = i;
            goto exit;
        }
    }

    // Не получилось привязать дескриптор к процессу, освободить память под file
    kfree(file);

exit:
    release_spinlock(&process->fd_spinlock);
    return fd;
}

int process_close_file(process_t* process, int fd)
{
    int rc = -1;
    acquire_spinlock(&process->fd_spinlock);
    file_t* file = process->fds[fd];
    if (file != NULL) {
        vfs_close(file->inode);
        kfree(file);
        process->fds[fd];
        rc = 0;
        goto exit;
    } else {
        rc = ERROR_BAD_FD;
    }

exit:
    release_spinlock(&process->fd_spinlock);
    return rc;
}

size_t process_read_file(process_t* process, int fd, char* buffer, size_t size)
{
    size_t bytes_read = 0;
    acquire_spinlock(&process->fd_spinlock);

    file_t* file = process->fds[fd];
    if (file->mode & FILE_OPEN_MODE_READ_ONLY) {
        struct inode* inode = file->inode;
        bytes_read = vfs_read(inode, file->pos, size, buffer);
        file->pos += bytes_read; 
    }

    release_spinlock(&process->fd_spinlock);

    return bytes_read;
}

int process_stat(process_t* process, int fd, struct stat* stat)
{
    int rc = -1;
    acquire_spinlock(&process->fd_spinlock);

    file_t* file = process->fds[fd];
    if (file != NULL) {
        struct inode* inode = file->inode;
        stat->st_ino = inode->inode;
        stat->st_size = inode->size;
        stat->st_mode = inode->mode;
        stat->st_uid = inode->uid;
        stat->st_gid = inode->gid;
        stat->st_atime = inode->access_time;
        stat->st_ctime = inode->create_time;
        stat->st_mtime = inode->modify_time;
        rc = 0;
    } else {
        rc = ERROR_BAD_FD;
    }

    release_spinlock(&process->fd_spinlock);

    return rc;
}

int process_readdir(process_t* process, int fd, struct dirent* dirent)
{
    int rc = -1;
    acquire_spinlock(&process->fd_spinlock);

    file_t* file = process->fds[fd];

    if (file != NULL) {

        struct inode* inode = file->inode;
        struct dirent* ndirent = vfs_readdir(inode, file->pos ++);
        
        if (ndirent == NULL) {
            return 0;
        }

        memcpy(dirent, ndirent, sizeof(struct dirent));
        kfree(ndirent);
        
        rc = 1;
    }

    release_spinlock(&process->fd_spinlock);

    return rc;
}

int process_get_working_dir(process_t* process, char* buffer, size_t size)
{
    memcpy(buffer, process->cur_dir, size);
    return 0;
}

int process_set_working_dir(process_t* process, const char* buffer)
{
    size_t buffer_length = strlen(buffer);
    memcpy(process->cur_dir, buffer, buffer_length);
    return 0;
}