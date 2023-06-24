#include "process.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "fs/vfs/vfs.h"

int process_open_file(process_t* process, const char* path, int mode, int flags)
{
    int fd = -1;
    vfs_inode_t* inode = vfs_fopen(path, 0);

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
        vfs_inode_t* inode = file->inode;
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
        vfs_inode_t* inode = file->inode;
        stat->st_ino = inode->inode;
        stat->st_size = inode->size;
        stat->st_mode = inode->mode;
        stat->st_uid = inode->uid;
        stat->st_gid = inode->gid;
        stat->st_atime = inode->access_time;
        stat->st_ctime = inode->create_time;
        stat->st_mtime = inode->modify_time;
        rc = 0;
    }

    release_spinlock(&process->fd_spinlock);

    return rc;
}