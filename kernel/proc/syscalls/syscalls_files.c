#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "errors.h"
#include "proc/process.h"
#include "cpu/cpu_local_x64.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "string.h"
#include "ipc/pipe.h"
#include "fs/vfs/superblock.h"

int sys_open_file(int dirfd, const char* path, int flags, int mode)
{
    int fd = -1;
    struct dentry* dir_dentry = NULL;
    struct process* process = cpu_get_current_thread()->process;

    // Получить dentry, относительно которой открыть файл
    fd = process_get_relative_direntry(process, dirfd, path, &dir_dentry);

    if (fd != 0) {
        // Не получилось найти директорию с файлом
        return fd;
    }

    // Открыть файл в ядре
    struct file* file = file_open(dir_dentry, path, flags, mode);

    if (file == NULL) {
        // Файл не найден
        return -ERROR_NO_FILE;
    }

    if ( !(file->inode->mode & INODE_TYPE_DIRECTORY) && (flags & FILE_OPEN_FLAG_DIRECTORY)) {
        // Мы пытаемся открыть обычный файл с флагом директории - выходим
        file_close(file);
        return -ERROR_NOT_A_DIRECTORY;
    }

    if ((file->inode->mode & INODE_TYPE_DIRECTORY) && file_allow_write(file)) {
        // Мы пытаемся открыть директорию с правами на запись
        file_close(file);
        return -ERROR_IS_DIRECTORY;
    }

    // Добавить файл к процессу
    fd = process_add_file(process, file);

    if (fd < 0) {
        // Не получилось привязать дескриптор к процессу, закрыть файл
        file_close(file);
    }

    return fd;
}

int sys_close_file(int fd)
{
    struct process* process = cpu_get_current_thread()->process;
    return process_close_file(process, fd);
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

    // Форматировать путь + дублировать путь
    char *formatted_path = format_path(path); 

    char* directory_path = NULL;
    char* filename = NULL;

    // Разделить путь на путь директории и имя файла
    split_path(formatted_path, &directory_path, &filename);

    // Открываем inode директории
    struct inode* dir_inode = vfs_fopen(dir_dentry, directory_path, NULL);

    if (directory_path) {
        kfree(directory_path);
    }

    if (dir_inode == NULL) {
        // Отсутствует полный путь к директории
        rc = -ERROR_NO_FILE;
        goto exit;
    }

    // Создать папку
    rc = inode_mkdir(dir_inode, filename, mode);
    // Закрыть inode директории
    inode_close(dir_inode);

exit:
    kfree(formatted_path);
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

    // Открыть файл относительно папки и флагов
    rc = process_open_file_relative(process, dirfd, filepath, flags, &file, &close_at_end);
    if (rc != 0) {
        return rc;
    }

    struct inode* inode = file->inode;
    rc = inode_stat(inode, statbuf);

    if (close_at_end) {
        file_close(file);
    }

    return rc;
}

int sys_set_mode(int dirfd, const char* filepath, mode_t mode, int flags)
{
    int rc = -1;
    int close_at_end = 0;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = NULL;
    
    // Открыть файл относительно папки и флагов
    rc = process_open_file_relative(process, dirfd, filepath, flags, &file, &close_at_end);
    if (rc != 0) {
        return rc;
    }

    struct inode* inode = file->inode;
    rc = inode_chmod(inode, mode);

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

int sys_remove_file(int dirfd, const char* path, int flags)
{
    printf_stdout("removing %s \n", path);

    struct process* process = cpu_get_current_thread()->process;
    
    return -1;
}

int sys_rename(int olddirfd, const char* oldpath, int newdirfd, const char* newpath, int flags)
{
    if (oldpath == NULL || newpath == NULL) {
        return -ERROR_NO_FILE;
    }

    struct process* process = cpu_get_current_thread()->process;

    // Получить dentry, относительно которой открыть старый файл
    struct dentry* olddir_dentry = NULL;
    struct dentry* newdir_dentry = NULL;

    // Получить dentry для olddirfd
    int rc = process_get_relative_direntry(process, olddirfd, oldpath, &olddir_dentry);
    if (rc != 0) 
        return rc;
    // Получить dentry для newdirfd
    rc = process_get_relative_direntry(process, newdirfd, newpath, &newdir_dentry);
    if (rc != 0) 
        return rc;

    // Открыть старый файл - получить inode и dentry
    struct dentry* old_dentry;
    struct inode* old_inode = vfs_fopen(olddir_dentry, oldpath, &old_dentry);
    if (old_inode == NULL || old_dentry == NULL) {
        return -ERROR_NO_FILE;
    }

    // Получить inode - директорию старого имени файла
    struct inode* old_parent_inode = vfs_fopen_parent(old_dentry);

    // Новый путь
    char* new_directory_path = NULL;
    // Новое имя
    char* new_filename = NULL;
    // Разделить новый путь на путь директории и имя файла
    split_path(newpath, &new_directory_path, &new_filename);

    // inode директории нового пути
    struct dentry* new_parent_dentry = NULL;
    struct inode* new_parent_inode = vfs_fopen(newdir_dentry, new_directory_path, &new_parent_dentry);

    if (old_inode->device != new_parent_inode->device) {
        rc = ERROR_OTHER_DEVICE;
        goto exit;
    }

    // Переместить на диске
    if (old_parent_inode->operations->rename) {
        rc = old_parent_inode->operations->rename(old_parent_inode, old_dentry, new_parent_inode, new_filename);

        if (rc != 0)
            goto exit;
    }

    // Поменять имя в dentry
    strcpy(old_dentry->name, new_filename);

    dentry_reparent(old_dentry, new_parent_dentry);

exit:
    DENTRY_CLOSE_SAFE(old_dentry)

    INODE_CLOSE_SAFE(old_parent_inode)

    INODE_CLOSE_SAFE(old_inode)
    
    if (new_directory_path)
        kfree(new_directory_path);

    return rc;
}