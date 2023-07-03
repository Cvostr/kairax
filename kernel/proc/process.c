#include "process.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "fs/vfs/vfs.h"
#include "string.h"
#include "thread.h"
#include "thread_scheduler.h"

void free_process(process_t* process)
{

}

int process_open_file(process_t* process, const char* path, int mode, int flags)
{
    int fd = -1;
    struct inode* inode = vfs_fopen(path, 0);

    if (inode == NULL) {
        // TODO: обработать для отсутствия файла ENOENT
    }

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

    if (fd >= MAX_DESCRIPTORS) {
        // set to errno  ERROR_BAD_FD;
        goto exit;
    }

    file_t* file = process->fds[fd];
    if (file != NULL) {
        inode_close(file->inode);
        kfree(file);
        process->fds[fd];
        rc = 0;
        goto exit;
    } else {
        //set to errno ERROR_BAD_FD;
    }

exit:
    release_spinlock(&process->fd_spinlock);
    return rc;
}

ssize_t process_read_file(process_t* process, int fd, char* buffer, size_t size)
{
    size_t bytes_read = -1;
    acquire_spinlock(&process->fd_spinlock);

    file_t* file = process->fds[fd];

    if (file == NULL) {
        //TODO: set errno to EBADF
        goto exit;
    }

    if (file->mode & FILE_OPEN_MODE_READ_ONLY) {
        struct inode* inode = file->inode;
        bytes_read = inode_read(inode, &file->pos, size, buffer);
    }

exit:
    release_spinlock(&process->fd_spinlock);

    return bytes_read;
}

off_t process_file_seek(process_t* process, int fd, off_t offset, int whence)
{
    off_t result = -1;
    acquire_spinlock(&process->fd_spinlock);

    file_t* file = process->fds[fd];
    if (file != NULL) {
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
        release_spinlock(&file->spinlock);
    }

    release_spinlock(&process->fd_spinlock);

    return result;
}

int process_stat(process_t* process, int fd, struct stat* stat)
{
    int rc = -1;
    acquire_spinlock(&process->fd_spinlock);

    file_t* file = process->fds[fd];
    if (file != NULL) {
        struct inode* inode = file->inode;
        inode_stat(inode, stat);
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
        struct dirent* ndirent = inode_readdir(inode, file->pos ++);
        
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

int process_create_thread(process_t* process, void* entry_ptr, void* arg, pid_t* tid, size_t stack_size)
{
    thread_t* thread = create_thread(process, entry_ptr, arg, stack_size);

    if (thread == NULL) {
        return -1;
    }

    if (tid != NULL) {
        *tid = thread->id;
    }

    scheduler_add_thread(thread);

    return 0;
}