#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "errors.h"
#include "proc/process.h"
#include "cpu/cpu_local.h"
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
    struct file* file = NULL;

    VALIDATE_USER_STRING(process, path)

    // Получить dentry, относительно которой открыть файл
    fd = process_get_relative_direntry1(process, dirfd, path, &dir_dentry);

    if (fd != 0) {
        // Не получилось найти директорию с файлом
        goto exit;
    }

    // Открыть файл в ядре
    fd = file_open_ex(dir_dentry, path, flags, mode, &file);

    if (fd != 0) 
    {
        // Ошибка, код уже записан в fd, так что сразу просто выходим
        goto exit;
    }

    int is_directory = (file->inode->mode & INODE_TYPE_MASK) == INODE_TYPE_DIRECTORY;

    if ( !is_directory && (flags & FILE_OPEN_FLAG_DIRECTORY)) {
        // Мы пытаемся открыть обычный файл с флагом директории - выходим
        file_close(file);
        fd = -ERROR_NOT_A_DIRECTORY;
        goto exit;
    }

    if (is_directory && file_allow_write(file)) {
        // Мы пытаемся открыть директорию с правами на запись
        file_close(file);
        fd = -ERROR_IS_DIRECTORY;
        goto exit;
    }

    // Проверка разрешений
    if ( (file_allow_read(file) && inode_check_perm(file->inode, process->euid, process->egid, S_IRUSR, S_IRGRP, S_IROTH) == 0) ||
        (file_allow_write(file) && inode_check_perm(file->inode, process->euid, process->egid, S_IWUSR, S_IWGRP, S_IWOTH) == 0) )
    {
        file_close(file);
        fd = -EACCES;
        goto exit;
    }

    // TODO: может делать это после попытки добавить к процессу?
    if ((file->inode->mode & INODE_TYPE_FILE) && (flags & FILE_OPEN_FLAG_TRUNCATE) && file_allow_write(file)) {
        inode_truncate(file->inode);
    }

    // Добавить файл к процессу
    fd = process_add_file(process, file);

    if (fd < 0) {
        // Не получилось привязать дескриптор к процессу, закрыть файл
        file_close(file);
        goto exit;
    }

    // Если передан флаг O_CLOEXEC, надо взвести бит дескриптора
    if ((flags & O_CLOEXEC) == O_CLOEXEC) 
    {
        process_set_cloexec(process, fd, 1);
    }

exit:
    DENTRY_CLOSE_SAFE(dir_dentry);

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
    VALIDATE_USER_STRING(process, path)

    // Получить dentry, относительно которой создать папку
    rc = process_get_relative_direntry1(process, dirfd, path, &dir_dentry);
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
    DENTRY_CLOSE_SAFE(dir_dentry);
    kfree(formatted_path);
    return rc;
}

ssize_t sys_read_file(int fd, char* buffer, size_t size)
{
    ssize_t bytes_read = -1;
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    if (thread->is_userspace) {
        // Эта функция также используется в ядре
        // Проверять диапазон, только если вызвана из userspace
        VALIDATE_USER_POINTER_PROTECTION(process, buffer, size, PAGE_PROTECTION_WRITE_ENABLE)
    }

    // Получить файл с увеличением счетчика ссылок
    struct file* file = process_get_file_ex(process, fd, TRUE);

    if (file == NULL) {
        bytes_read = -ERROR_BAD_FD;
        goto exit;
    }

    // Чтение из файла
    bytes_read = file_read(file, size, buffer);
    // Уменьшить счетчик ссылок (и может быть закрыть файл)
    file_close(file);
exit:
    return bytes_read;
}

ssize_t sys_write_file(int fd, const char* buffer, size_t size)
{
    ssize_t bytes_written = -1;
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    if (thread->is_userspace) {
        // Эта функция также используется в ядре
        // Проверять диапазон, только если вызвана из userspace
        VALIDATE_USER_POINTER(process, buffer, size)
    }

    // Получить файл с увеличением счетчика ссылок
    struct file* file = process_get_file_ex(process, fd, TRUE);

    if (file == NULL) {
        bytes_written = -ERROR_BAD_FD;
        goto exit;
    }
    
    // Записать в файл
    bytes_written = file_write(file, size, buffer);
    // Уменьшить счетчик ссылок (и может быть закрыть файл)
    file_close(file);

exit:
    return bytes_written;
}

int sys_stat(int dirfd, const char* filepath, struct stat* statbuf, int flags)
{
    int rc = -1;
    int close_at_end = 0;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = NULL;

    // Путь может быть пустым
    if (filepath != NULL)
    {
        // Проверить адрес строки пути файла
        VALIDATE_USER_STRING(process, filepath)
    }

    // Проверить адрес вывода
    VALIDATE_USER_POINTER_PROTECTION(process, statbuf, sizeof(struct stat), PAGE_PROTECTION_WRITE_ENABLE)

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

    // Путь может быть пустым
    if (filepath != NULL)
    {
        // Проверить адрес строки пути файла
        VALIDATE_USER_STRING(process, filepath)
    }
    
    // Открыть файл относительно папки и флагов
    rc = process_open_file_relative(process, dirfd, filepath, flags, &file, &close_at_end);
    if (rc != 0) {
        return rc;
    }

    struct inode* inode = file->inode;

    if (inode->uid == process->euid || process->euid == 0)
    {
        rc = inode_chmod(inode, mode);
    } else 
    {
        rc = -EPERM;
    }

    if (close_at_end) {
        file_close(file);
    }

    return rc;
}

int sys_readdir(int fd, struct dirent* dirent)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

    VALIDATE_USER_POINTER_PROTECTION(process, dirent, sizeof(struct dirent), PAGE_PROTECTION_WRITE_ENABLE)

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

        if (file->inode != NULL) {
            if (((file->inode->mode & INODE_FLAG_PIPE) == INODE_FLAG_PIPE) ||
                ((file->inode->mode & INODE_FLAG_SOCKET) == INODE_FLAG_SOCKET)) {
                return -ESPIPE;
            }
        }

        result = file_seek(file, offset, whence);

    } else {
        result = -ERROR_BAD_FD;
    }

    return result;
}

int sys_unlink(int dirfd, const char* path, int flags)
{
    struct process* process = cpu_get_current_thread()->process;

    // Проверить адрес строки пути файла
    VALIDATE_USER_STRING(process, path)

    // Получение dentry от dirfd для относительного поиска
    struct dentry* dirdentry = NULL;
    int rc = process_get_relative_direntry1(process, dirfd, path, &dirdentry);
    if (rc != 0) 
        return rc;

    // Получить удаляемую inode и dentry
    struct dentry* target_dentry = NULL;
    // Флаги O_NOFOLLOW | O_PATH нужны для того, чтобы удалить именно символическую ссылку, а не то, на что она указывает
    struct inode* target_inode = vfs_fopen_ex(dirdentry, path, &target_dentry, O_NOFOLLOW | O_PATH);
    
    // не нашли файл, который нужно удалить
    if (target_inode == NULL || target_dentry == NULL) {
        rc = -ERROR_NO_FILE;
        goto exit;
    }

    int ino_mode = target_inode->mode;
    INODE_CLOSE_SAFE(target_inode)

    // Проверка, не пытаемся ли удалить точку монтирования?
    if ((target_dentry->flags & DENTRY_MOUNTPOINT) == DENTRY_MOUNTPOINT) {
        rc = -ERROR_BUSY;
        goto exit;
    }

    // Нельзя удалять директории
    if ((ino_mode & INODE_TYPE_MASK) == INODE_TYPE_DIRECTORY) 
    {
        rc = -ERROR_IS_DIRECTORY;
        goto exit;
    }

    // Пометить dentry для удаления
    target_dentry->flags |= DENTRY_INVALID;
    
    // Выполнить unlink в драйвере ФС
    rc = inode_unlink(target_dentry->parent->d_inode, target_dentry);

    if (rc != 0)
        goto exit;

exit:
    // Закрыть удаляемую dentry
    //printk("unlink() closing dentry %s, refs %i\n", target_dentry->name, target_dentry->refs_count);
    DENTRY_CLOSE_SAFE(target_dentry);

    DENTRY_CLOSE_SAFE(dirdentry)

    return rc;
}

int sys_rmdir(const char* path)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

    // Проверить адрес строки пути файла
    VALIDATE_USER_STRING(process, path)

    // Получить удаляемую inode и dentry относительно рабочей директории
    struct dentry* target_dentry = NULL;
    struct inode* target_inode = vfs_fopen(process->pwd, path, &target_dentry);

    // не нашли файл, который нужно удалить
    if (target_inode == NULL || target_dentry == NULL) {
        rc = -ERROR_NO_FILE;
        goto exit;
    }

    int ino_mode = target_inode->mode;
    INODE_CLOSE_SAFE(target_inode)

    // С помощью rmdir удаляем только директории
    if ( (ino_mode & INODE_TYPE_MASK) != INODE_TYPE_DIRECTORY) {
        rc = -ERROR_NOT_A_DIRECTORY;
        goto exit;
    }

    // Проверка, не пытаемся ли удалить точку монтирования?
    if ((target_dentry->flags & DENTRY_MOUNTPOINT) == DENTRY_MOUNTPOINT) {
        rc = -ERROR_BUSY;
        goto exit;
    }
/*
    // нельзя удалять . и ..
    if (strcmp(target_dentry->name, ".") == 0 || strcmp(target_dentry->name, "..") == 0) {
        rc = -ERROR_INVALID_VALUE;
        goto exit;
    }
*/

    // Проверить, что удаляемая директория не является рабочей папкой какого либо процесса
    if (process_list_is_dentry_used_as_cwd(target_dentry) != 0) {
        rc = -ERROR_BUSY;
        goto exit;
    }

    // Пометить dentry для удаления
    target_dentry->flags |= DENTRY_INVALID;

    // Выполнение rmdir в драйвере ФС
    struct inode* parent_inode = target_dentry->parent->d_inode;
    rc = inode_rmdir(parent_inode, target_dentry);
    if (rc != 0)
        goto exit;

exit:
    // Закрыть dentry
    DENTRY_CLOSE_SAFE(target_dentry);

    return rc;
}

int sys_linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags)
{
    if (oldpath == NULL || newpath == NULL) 
    {
        return -ERROR_NO_FILE;
    }

    struct process* process = cpu_get_current_thread()->process;


    // Проверить адреса строк путей файлов
    VALIDATE_USER_STRING(process, oldpath)
    VALIDATE_USER_STRING(process, newpath)

    // Новый путь
    char* new_directory_path = NULL;
    // Новое имя
    char* new_filename = NULL;

    struct dentry* olddir_dentry = NULL;
    struct dentry* newdir_dentry = NULL;

    struct inode* old_parent_inode = NULL;
    struct inode* new_parent_inode = NULL;
    struct dentry* new_parent_dentry = NULL;

    // Получить dentry для olddirfd
    int rc = process_get_relative_direntry1(process, olddirfd, oldpath, &olddir_dentry);
    if (rc != 0) 
        return rc;
    // Получить dentry для newdirfd
    rc = process_get_relative_direntry1(process, newdirfd, newpath, &newdir_dentry);
    if (rc != 0) 
        return rc;

    // Открыть источник файл - получить inode и dentry
    struct dentry* old_dentry = NULL;
    struct inode* old_inode = vfs_fopen(olddir_dentry, oldpath, &old_dentry);
    if (old_inode == NULL || old_dentry == NULL) 
    {
        rc = -ERROR_NO_FILE;
        goto exit;
    }

    // Запрещено делать жесткие ссылки на директории
    if ((old_inode->mode & INODE_TYPE_MASK) == INODE_TYPE_DIRECTORY) 
    {
        rc = -EPERM;
        goto exit;
    }

    // Получить inode - директорию старого файла
    old_parent_inode = vfs_fopen_parent(old_dentry);

    // Разделить новый путь на путь директории и имя файла
    split_path(newpath, &new_directory_path, &new_filename);

    // Открыть inode директории нового пути
    new_parent_inode = vfs_fopen(newdir_dentry, new_directory_path, &new_parent_dentry);

    // Инода, старый и новый родители должны быть на одном устройстве
    if (old_inode->device != new_parent_inode->device || old_parent_inode->device != new_parent_inode->device) 
    {
        rc = -ERROR_OTHER_DEVICE;
        goto exit;
    }

    rc = inode_linkat(old_dentry, new_parent_inode, new_filename);
    if (rc != 0)
        goto exit;    

exit:
    // Освобождаем в обратном порядке
    DENTRY_CLOSE_SAFE(new_parent_dentry)
    INODE_CLOSE_SAFE(new_parent_inode)
    INODE_CLOSE_SAFE(old_parent_inode)
    DENTRY_CLOSE_SAFE(old_dentry)
    
    DENTRY_CLOSE_SAFE(newdir_dentry)
    DENTRY_CLOSE_SAFE(olddir_dentry)

    if (new_directory_path) {
        kfree(new_directory_path);
    }

    return rc;
}

int sys_symlinkat(const char *target, int newdirfd, const char *linkpath)
{
    if (target == NULL || linkpath == NULL) 
    {
        return -ERROR_NO_FILE;
    }

    int rc;
    struct process* process = cpu_get_current_thread()->process;

    // dentry директории из файлового дескриптора
    struct dentry* newdir_dentry = NULL;
    struct dentry* new_parent_dentry = NULL;

    // Проверить адрес строки пути цели
    VALIDATE_USER_STRING(process, target)
    // Проверить адрес строки пути ссылки
    VALIDATE_USER_STRING(process, linkpath)

    // Получить dentry для newdirfd
    rc = process_get_relative_direntry1(process, newdirfd, linkpath, &newdir_dentry);
    if (rc != 0) 
        return rc;

    // Новый путь
    char* new_directory_path = NULL;
    // Новое имя
    char* new_filename = NULL;
    // Разделить новый путь на путь директории и имя файла
    split_path(linkpath, &new_directory_path, &new_filename);

    // Открыть inode директории нового пути
    struct inode* new_parent_inode = vfs_fopen(newdir_dentry, new_directory_path, &new_parent_dentry);
    if (new_parent_inode == NULL || new_parent_dentry == NULL) 
    {
        rc = -ERROR_NO_FILE;
        goto exit;
    }

    rc = inode_symlink(new_parent_inode, new_filename, target);

exit:

    DENTRY_CLOSE_SAFE(new_parent_dentry)
    INODE_CLOSE_SAFE(new_parent_inode)

    DENTRY_CLOSE_SAFE(newdir_dentry)

    if (new_directory_path) {
        kfree(new_directory_path);
    }

    return rc;
}

ssize_t sys_readlinkat(int dirfd, const char* pathname, char* buf, size_t bufsize)
{
    int rc;
    struct process* process = cpu_get_current_thread()->process;

    VALIDATE_USER_POINTER_PROTECTION(process, buf, bufsize, PAGE_PROTECTION_WRITE_ENABLE)
    //
    VALIDATE_USER_STRING(process, pathname)

    struct dentry* directory_dentry = NULL;
    struct dentry* symlink_dentry = NULL;

    // Получить dentry для newdirfd
    rc = process_get_relative_direntry1(process, dirfd, pathname, &directory_dentry);
    if (rc != 0) 
        return rc;

    struct inode* symlink_inode = vfs_fopen_ex(directory_dentry, pathname, &symlink_dentry, O_NOFOLLOW | O_PATH);
    if (symlink_inode == NULL || symlink_dentry == NULL) 
    {
        rc = -ERROR_NO_FILE;
        goto exit;
    }

    if ((symlink_inode->mode & INODE_TYPE_MASK) != INODE_FLAG_SYMLINK) 
    {
        rc = -ERROR_INVALID_VALUE;
        goto exit;
    }

    rc = inode_readlink(symlink_inode, buf, bufsize);

exit:

    DENTRY_CLOSE_SAFE(symlink_dentry)
    INODE_CLOSE_SAFE(symlink_inode)

    DENTRY_CLOSE_SAFE(directory_dentry)

    return rc;
}

int sys_mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev)
{
    if (pathname == NULL) 
    {
        return -ERROR_NO_FILE;
    }

    struct process* process = cpu_get_current_thread()->process;

    // проверить адрес строки пути
    VALIDATE_USER_STRING(process, pathname)

    // dentry от dirfd
    struct dentry* dir_dentry = NULL;
    // dentry и inode родителя
    struct dentry* parent_dentry = NULL;
    struct inode* parent_inode = NULL;
    // Новый путь
    char* new_directory_path = NULL;
    // Новое имя
    char* new_filename = NULL;

    // Получить dentry для olddirfd
    int rc = process_get_relative_direntry1(process, dirfd, pathname, &dir_dentry);
    if (rc != 0) 
        return rc;

    // Разделить новый путь на путь директории и имя файла
    split_path(pathname, &new_directory_path, &new_filename);

    // Открыть inode директории нового пути
    parent_inode = vfs_fopen(dir_dentry, new_directory_path, &parent_dentry);

    // выполнить операцию в ФС
    rc = inode_mknod(parent_inode, new_filename, mode);
    
    INODE_CLOSE_SAFE(parent_inode)
    DENTRY_CLOSE_SAFE(parent_dentry)
    DENTRY_CLOSE_SAFE(dir_dentry)
    if (new_directory_path) {
        kfree(new_directory_path);
    }

    return rc;
}

int sys_rename(int olddirfd, const char* oldpath, int newdirfd, const char* newpath, int flags)
{
    if (oldpath == NULL || newpath == NULL) {
        return -ERROR_NO_FILE;
    }

    struct process* process = cpu_get_current_thread()->process;

    // Проверить адреса строк
    VALIDATE_USER_STRING(process, oldpath)
    VALIDATE_USER_STRING(process, newpath)

    struct dentry* olddir_dentry = NULL;
    struct dentry* newdir_dentry = NULL;
    struct dentry* new_parent_dentry = NULL;

    struct inode* old_parent_inode = NULL;
    struct inode* new_parent_inode = NULL;

    // Новый путь
    char* new_directory_path = NULL;
    // Новое имя
    char* new_filename = NULL;

    // Получить dentry для olddirfd
    int rc = process_get_relative_direntry1(process, olddirfd, oldpath, &olddir_dentry);
    if (rc != 0) 
        return rc;
    // Получить dentry для newdirfd
    rc = process_get_relative_direntry1(process, newdirfd, newpath, &newdir_dentry);
    if (rc != 0) 
        return rc;

    // Открыть старый файл - получить inode и dentry
    struct dentry* old_dentry = NULL;
    struct inode* old_inode = vfs_fopen(olddir_dentry, oldpath, &old_dentry);
    if (old_inode == NULL || old_dentry == NULL) {
        rc = -ERROR_NO_FILE;
        goto exit;
    }

    // Получить inode - директорию старого файла
    old_parent_inode = vfs_fopen_parent(old_dentry);

    // Разделить новый путь на путь директории и имя файла
    split_path(newpath, &new_directory_path, &new_filename);

    if (strcmp(new_filename, "..") == 0 || strcmp(new_filename, ".") == 0) {
        rc = -ERROR_NO_FILE;
        goto exit;
    }

    // Открыть inode директории нового пути
    new_parent_inode = vfs_fopen(newdir_dentry, new_directory_path, &new_parent_dentry);

    if (new_parent_inode == NULL || new_parent_dentry == NULL) {
        rc = -ERROR_NO_FILE;
        goto exit;
    }

    if (dentry_is_child(old_dentry, new_parent_dentry) == 1 || old_dentry == new_parent_dentry) {
        rc = -ERROR_NO_FILE;
        goto exit;
    }

    // Инода, старый и новый родители должны быть на одном устройстве
    if (old_inode->device != new_parent_inode->device || old_parent_inode->device != new_parent_inode->device) {
        rc = -ERROR_OTHER_DEVICE;
        goto exit;
    }

    // Переместить на диске
    rc = inode_rename(old_parent_inode, old_dentry, new_parent_inode, new_filename);
    if (rc != 0)
        goto exit;    

    // Поменять имя в dentry
    strcpy(old_dentry->name, new_filename);

    dentry_reparent(old_dentry, new_parent_dentry);

exit:
    // Освобождаем в обратном порядке
    DENTRY_CLOSE_SAFE(new_parent_dentry)
    INODE_CLOSE_SAFE(new_parent_inode)

    INODE_CLOSE_SAFE(old_parent_inode)
    INODE_CLOSE_SAFE(old_inode)
    DENTRY_CLOSE_SAFE(old_dentry)

    DENTRY_CLOSE_SAFE(newdir_dentry)
    DENTRY_CLOSE_SAFE(olddir_dentry)
    
    if (new_directory_path)
        kfree(new_directory_path);

    return rc;
}