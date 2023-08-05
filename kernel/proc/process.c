#include "process.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "fs/vfs/vfs.h"
#include "string.h"
#include "thread.h"
#include "thread_scheduler.h"
#include "memory/kernel_vmm.h"

void free_process(struct process* process)
{
    for (unsigned int i = 0; i < list_size(process->threads); i ++) {
        struct thread* thread = list_get(process->threads, i);
        kfree(thread);
    }

    if (process->tls) {
        kfree(process->tls);
    }

    // Освободить память списков
    free_list(process->threads);
    free_list(process->children);

    // Закрыть незакрытые файловые дескрипторы
    for (int i = 0; i < MAX_DESCRIPTORS; i ++) {
        if (process->fds[i] != NULL) {
            file_close(process->fds[i]);
        }
    }

    // Закрыть объекты рабочей папки
    if (process->workdir) {
        file_close(process->workdir);
    }
    
    // Уничтожение таблицы виртуальной памяти процесса
    // и освобождение занятых таблиц физической
    //vmm_destroy_root_page_table(process->vmemory_table);

    kfree(process);

    scheduler_from_killed();
}

struct file* process_get_file(struct process* process, int fd)
{
    struct file* result = NULL;
    acquire_spinlock(&process->fd_spinlock);

    if (fd < 0 || fd >= MAX_DESCRIPTORS)
        goto exit;

    result = process->fds[fd];

exit:
    release_spinlock(&process->fd_spinlock);
    return result;
}

int process_create_process(struct process* process, const char* filepath, struct process_create_info* info)
{
    struct inode* inode = vfs_fopen(NULL, filepath, 0, NULL);
    if (inode == NULL) {
        return -1;
    }

    loff_t offset = 0;
    int size = inode->size;
    char* image_data = kmalloc(size);
    inode_read(inode, &offset, size, image_data);

    int rc = create_new_process_from_image(process, image_data, info);

exit:

    // Закрыть inode, освободить память
    inode_close(inode);
    kfree(image_data);

    return rc;
}

int process_open_file(struct process* process, int dirfd, const char* path, int flags, int mode)
{
    int fd = -1;
    struct dentry* dir_dentry = NULL;

    if (dirfd == FD_CWD && process->workdir) {
        // Открыть относительно рабочей директории
        dir_dentry = process->workdir->dentry;
        
    } else if (!vfs_is_path_absolute(path)) {
        // Открыть относительно другого файла
        // Получить файл по дескриптору
        struct file* dirfile = process_get_file(process, dirfd);

        if (dirfile) {
            if ( !(dirfile->inode->mode & INODE_TYPE_DIRECTORY)) {
                fd = -ERROR_NOT_A_DIRECTORY;
                goto exit;
            }
            // проверить тип inode
            dir_dentry = dirfile->dentry;
        } else {
            // Файл не нашелся, а путь не является абсолютным - выходим
            return ERROR_BAD_FD;
        }
    }

    struct file* file = file_open(dir_dentry, path, flags, mode);

    if (file == NULL) {
        // Файл не найден
        return -ERROR_NO_FILE;
    }

    if ( !(file->inode->mode & INODE_TYPE_DIRECTORY) && (flags & FILE_OPEN_FLAG_DIRECTORY)) {
        // Мы пытаемся открыть обыйчный файл с флагом директории - выходим
        file_close(file);
        return -ERROR_NOT_A_DIRECTORY;
    }

    if ((file->inode->mode & INODE_TYPE_DIRECTORY) && (flags & FILE_OPEN_MODE_WRITE_ONLY)) {
        // Мы пытаемся открыть директорию с правами на запись
        file_close(file);
        return -ERROR_IS_DIRECTORY;
    }

    // Найти свободный номер дескриптора для процесса
    acquire_spinlock(&process->fd_spinlock);

    for (int i = 0; i < MAX_DESCRIPTORS; i ++) {
        if (process->fds[i] == NULL) {
            process->fds[i] = file;
            fd = i;
            goto exit;
        }
    }

    fd = -ERROR_TOO_MANY_OPEN_FILES;

    // Не получилось привязать дескриптор к процессу, закрыть файл
    file_close(file);

exit:
    release_spinlock(&process->fd_spinlock);
    return fd;
}

int process_close_file(struct process* process, int fd)
{
    int rc = -1;
    struct file* file = process_get_file(process, fd);

    acquire_spinlock(&process->fd_spinlock);
    if (file != NULL) {
        file_close(file);
        process->fds[fd] = NULL;
        rc = 0;
        goto exit;
    } else {
        rc = -ERROR_BAD_FD;
    }

exit:
    release_spinlock(&process->fd_spinlock);
    return rc;
}

ssize_t process_read_file(struct process* process, int fd, char* buffer, size_t size)
{
    ssize_t bytes_read = -1;
    struct file* file = process_get_file(process, fd);

    if (file == NULL) {
        bytes_read = -ERROR_BAD_FD;
        goto exit;
    }

    // Чтение из файла
    bytes_read = file_read(file, size, buffer);
exit:
    return bytes_read;
}

ssize_t process_write_file(struct process* process, int fd, const char* buffer, size_t size)
{
    ssize_t bytes_written = -1;
    struct file* file = process_get_file(process, fd);

    if (file == NULL) {
        bytes_written = -ERROR_BAD_FD;
        goto exit;
    }

    // Записать в файл
    bytes_written = file_write(file, size, buffer);

exit:
    return bytes_written;
}

off_t process_file_seek(struct process* process, int fd, off_t offset, int whence)
{
    off_t result = -1;

    struct file* file = process_get_file(process, fd);

    if (file != NULL) {
        result = file_seek(file, offset, whence);
    }

    return result;
}

int process_stat(struct process* process, int fd, struct stat* stat)
{
    int rc = -1;

    struct file* file = process_get_file(process, fd);

    if (file != NULL) {
        struct inode* inode = file->inode;
        inode_stat(inode, stat);
        rc = 0;
    } else {
        rc = ERROR_BAD_FD;
    }

    return rc;
}

int process_readdir(struct process* process, int fd, struct dirent* dirent)
{
    int rc = -1;
    struct file* file = process_get_file(process, fd);

    if (file != NULL) {
        // Вызов readdir из файла   
        rc = file_readdir(file, dirent);
    } else {
        
        rc = -ERROR_BAD_FD;
    }

exit:

    return rc;
}

int process_get_working_dir(struct process* process, char* buffer, size_t size)
{
    int rc = -1;
    if (process->workdir) {
        
        size_t reqd_size = 0;
        vfs_dentry_get_absolute_path(process->workdir->dentry, &reqd_size, NULL);
        if (reqd_size + 1 > size) {
            goto exit;
        }

        vfs_dentry_get_absolute_path(process->workdir->dentry, NULL, buffer);
        rc = 0;
    }

exit:
    return rc;
}

int process_set_working_dir(struct process* process, const char* buffer)
{
    int rc = -1;

    struct file* new_workdir = file_open(process->workdir->dentry, buffer, 0, 0);

    if (new_workdir) {
        // файл открыт, убеждаемся что это директория 
        if (new_workdir->inode->mode & INODE_TYPE_DIRECTORY) {

            //  Закрываем старый файл, если был
            if (process->workdir) {
                file_close(process->workdir);
            }

            process->workdir = new_workdir;

            rc = 0;
        } else {
            file_close(new_workdir);
        }
    }

    return rc;
}

int process_create_thread(struct process* process, void* entry_ptr, void* arg, pid_t* tid, size_t stack_size)
{
    struct thread* thread = create_thread(process, entry_ptr, arg, NULL, stack_size);

    if (thread == NULL) {
        return -1;
    }

    if (tid != NULL) {
        *tid = thread->id;
    }

    scheduler_add_thread(thread);

    return 0;
}