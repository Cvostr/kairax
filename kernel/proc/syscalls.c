#include "syscalls.h"
#include "fs/vfs/vfs.h"
#include "errors.h"
#include "process.h"
#include "cpu/cpu_local_x64.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"

int sys_open_file(int dirfd, const char* path, int flags, int mode)
{
    int fd = -1;
    struct dentry* dir_dentry = NULL;
    struct process* process = cpu_get_current_thread()->process;

    // Получить dentry, относительно которой открыть файл
    fd = process_get_relative_direntry(process, dirfd, path, &dir_dentry);
    if (fd != 0) {
        return fd;
    }
    /*if (dirfd == FD_CWD && process->workdir) {
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
    }*/

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


int sys_close_file(int fd)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;
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

int sys_mkdir(int dirfd, const char* path, int mode)
{
    int rc = -1;
    struct dentry* dir_dentry = NULL;
    struct process* process = cpu_get_current_thread()->process;

    // Получить dentry, относительно которой создать папку
    rc = process_get_relative_direntry(process, dirfd, path, &dir_dentry);
    if (rc != 0) {
        return rc;
    }

    char* directory_path = NULL;
    char* filename = NULL;

    // Разделить путь на путь директории имя файла
    split_path(path, &directory_path, &filename);

    // Открываем inode директории
    struct inode* dir_inode = vfs_fopen(dir_dentry, directory_path, 0, NULL);

    if (directory_path) {
        kfree(directory_path);
    }

    if (dir_inode == NULL) {
        // Отсутствует полный путь к директории
        return -1;
    }

    // Создать папку
    rc = inode_mkdir(dir_inode, filename, mode);
    // Закрыть inode директории
    inode_close(dir_inode);

    return rc;
}

ssize_t sys_read_file(int fd, char* buffer, size_t size)
{
    ssize_t bytes_read = -1;
    struct process* process = cpu_get_current_thread()->process;
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

ssize_t sys_write_file(int fd, const char* buffer, size_t size)
{
    ssize_t bytes_written = -1;
    struct process* process = cpu_get_current_thread()->process;
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

int sys_stat(int dirfd, const char* filepath, struct stat* statbuf, int flags)
{
    int rc = -1;
    int close_at_end = 0;
    struct process* process = cpu_get_current_thread()->process;

    struct file* file = NULL;
    if (flags & DIRFD_IS_FD) {
        // Дескриптор файла передан в dirfd
        file = process_get_file(process, dirfd);

    } else if (dirfd == FD_CWD && process->workdir->dentry) {
        // Указан путь относительно рабочей директории
        file = file_open(process->workdir->dentry, filepath, 0, 0);
        close_at_end = 1;
    } else {
        // Открываем файл относительно dirfd
        struct file* dirfile = process_get_file(process, dirfd);

        if (dirfile == NULL) {
            // Не найден дескриптор dirfd
            return -ERROR_BAD_FD;
        }

        // Открыть файл
        file = file_open(dirfile->dentry, filepath, 0, 0);
        close_at_end = 1;
    }

    if (file == NULL) {
        // Не получилось открыть файл, выходим
        return -ERROR_BAD_FD;
    }

    struct inode* inode = file->inode;
    inode_stat(inode, statbuf);
    rc = 0;

    if (close_at_end) {
        file_close(file);
    }

    return rc;
}

int sys_readdir(int fd, struct dirent* dirent)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;
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

int sys_ioctl(int fd, uint64_t request, uint64_t arg)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

    struct file* file = process_get_file(process, fd);

    if (file != NULL) {
        rc = file_ioctl(file, request, arg);
    } else {
        rc = -ERROR_BAD_FD;
    }

    return rc;
}

off_t sys_file_seek(int fd, off_t offset, int whence)
{
    off_t result = -1;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = process_get_file(process, fd);

    if (file != NULL) {
        result = file_seek(file, offset, whence);
    }

    return result;
}

int sys_get_working_dir(char* buffer, size_t size)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

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

int sys_set_working_dir(const char* buffer)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;
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

void sys_exit_process(int code)
{
    struct process* process = cpu_get_current_thread()->process;
    scheduler_remove_process_threads(process);
    free_process(process);
}

pid_t sys_get_process_id()
{
    struct process* process = cpu_get_current_thread()->process;
    return process->pid;
}

pid_t sys_get_thread_id()
{
    struct thread* current_thread = cpu_get_current_thread();
    return current_thread->id;
}

int sys_thread_sleep(uint64_t time)
{
    struct thread* thread = cpu_get_current_thread();
    thread->state = THREAD_UNINTERRUPTIBLE; // Ожидающий системный вызов
    for (uint64_t i = 0; i < time; i ++) {
        scheduler_yield();
    }
    thread->state = THREAD_RUNNING;
}