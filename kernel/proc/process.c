#include "process.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "fs/vfs/vfs.h"
#include "string.h"
#include "thread.h"
#include "thread_scheduler.h"

void free_process(struct process* process)
{
    for (unsigned int i = 0; i < list_size(process->threads); i ++) {
        struct thread* thread = list_get(process->threads, i);
        kfree(thread);
    }

    if (process->tls) {
        kfree(process->tls);
    }

    free_list(process->threads);
    free_list(process->children);

    // Закрыть незакрытые файловые дескрипторы
    for (int i = 0; i < MAX_DESCRIPTORS; i ++) {
        if (process->fds[i] != NULL) {
            file_close(process->fds[i]);
        }
    }

    //vmm_destroy_root_page_table(process->vmemory_table);

    kfree(process);
}

int process_open_file(struct process* process, const char* path, int mode, int flags)
{
    int fd = -1;
    struct dentry* dentry;
    struct inode* inode = vfs_fopen(path, 0, &dentry);

    if (inode == NULL) {
        // TODO: обработать для отсутствия файла ENOENT
        return fd;
    }

    // Создать новый дескриптор файла
    file_t* file = new_file();
    file->inode = inode;
    file->mode = mode;
    file->flags = flags;
    file->pos = 0;
    file->dentry = dentry;

    // Найти свободный номер дескриптора для процесса
    acquire_spinlock(&process->fd_spinlock);

    for (int i = 0; i < MAX_DESCRIPTORS; i ++) {
        if (process->fds[i] == NULL) {
            process->fds[i] = file;
            fd = i;
            goto exit;
        }
    }

    // Не получилось привязать дескриптор к процессу, закрыть файл
    file_close(file);

exit:
    release_spinlock(&process->fd_spinlock);
    return fd;
}

int process_close_file(struct process* process, int fd)
{
    int rc = -1;
    acquire_spinlock(&process->fd_spinlock);

    if (fd >= MAX_DESCRIPTORS) {
        // set to errno  ERROR_BAD_FD;
        goto exit;
    }

    file_t* file = process->fds[fd];
    if (file != NULL) {
        file_close(file);
        process->fds[fd] = NULL;
        rc = 0;
        goto exit;
    } else {
        //set to errno ERROR_BAD_FD;
    }

exit:
    release_spinlock(&process->fd_spinlock);
    return rc;
}

ssize_t process_read_file(struct process* process, int fd, char* buffer, size_t size)
{
    ssize_t bytes_read = -1;
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

off_t process_file_seek(struct process* process, int fd, off_t offset, int whence)
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

int process_stat(struct process* process, int fd, struct stat* stat)
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

int process_readdir(struct process* process, int fd, struct dirent* dirent)
{
    int rc = -1;
    acquire_spinlock(&process->fd_spinlock);

    file_t* file = process->fds[fd];

    if (file != NULL) {
        acquire_spinlock(&file->spinlock);

        struct inode* inode = file->inode;
        
        struct dirent* ndirent = inode_readdir(inode, file->pos ++);
        
        release_spinlock(&file->spinlock);
        
        if (ndirent == NULL) {
            rc = 0;
            goto exit;
        }

        memcpy(dirent, ndirent, sizeof(struct dirent));
        kfree(ndirent);
        
        rc = 1;
    } else {
        rc = -1;
    }

exit:
    release_spinlock(&process->fd_spinlock);

    return rc;
}

int process_get_working_dir(struct process* process, char* buffer, size_t size)
{
    memcpy(buffer, process->cur_dir, size);
    return 0;
}

int process_set_working_dir(struct process* process, const char* buffer)
{
    size_t buffer_length = strlen(buffer);
    memcpy(process->cur_dir, buffer, buffer_length);
    return 0;
}

int process_create_thread(struct process* process, void* entry_ptr, void* arg, pid_t* tid, size_t stack_size)
{
    struct thread* thread = create_thread(process, entry_ptr, arg, stack_size);

    if (thread == NULL) {
        return -1;
    }

    if (tid != NULL) {
        *tid = thread->id;
    }

    scheduler_add_thread(thread);

    return 0;
}