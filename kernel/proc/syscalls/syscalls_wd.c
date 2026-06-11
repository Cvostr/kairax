#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "errors.h"
#include "proc/process.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "string.h"
#include "cpu/cpu_local_x64.h"

int sys_get_working_dir(char* buffer, size_t size)
{
    int rc = 0;
    size_t reqd_size = 0;
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    if (thread->is_userspace) {
        VALIDATE_USER_POINTER(process, buffer, size)
    }

    acquire_spinlock(&process->pwd_lock);
    if (process->pwd) {
        
        // Вычисляем необходимый размер буфера
        vfs_dentry_get_absolute_path(process->pwd, &reqd_size, NULL);
        
        if (reqd_size + 1 > size) {
            // Размер буфера недостаточный
            rc = -ERROR_RANGE;
            goto exit;
        }

        // Записываем путь в буфер
        vfs_dentry_get_absolute_path(process->pwd, NULL, buffer);
    }
    release_spinlock(&process->pwd_lock);

exit:
    return rc;
}

int sys_set_working_dir(const char* buffer)
{
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    if (thread->is_userspace) {
        VALIDATE_USER_STRING(process, buffer)
    }

    struct dentry* workdir_dentry = process->pwd != NULL ? process->pwd : NULL;
    struct dentry* new_workdir = vfs_dentry_traverse_path(workdir_dentry, buffer, 0);

    if (new_workdir == NULL) {
        return -ENOENT;
    }

    // Проверить тип (должна быть директория)
    if ((new_workdir->flags & DENTRY_TYPE_DIRECTORY) != DENTRY_TYPE_DIRECTORY) {
        // Это не директория
        return -ERROR_NOT_A_DIRECTORY;
    }

    // inode директории должно иметь права на исполнение
    if (inode_check_perm(new_workdir->d_inode, process->euid, process->egid, S_IXUSR, S_IXGRP, S_IXOTH) == 0)
    {
        return -EACCES;
    }
    
    // непосредственно, сама установка домашней директории
    process_set_workdir(process, new_workdir);

    return 0;
}

int sys_fchdir(int fd)
{   
    int rc = 0;
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    // Получить файл с увеличением счетчика ссылок
    struct file* file = process_get_file_ex(process, fd, TRUE);
    if (file == NULL) 
    {
        return -ERROR_BAD_FD;
    }

    struct inode *inode = file->inode;
    struct dentry *dentr = file->dentry;

    // inode и dentry обязательны
    if (inode == NULL || dentr == NULL)
    {
        rc = -ERROR_NOT_A_DIRECTORY;
        goto exit;
    }

    // проверим, что это директория
    if ((file->flags & FILE_OPEN_FLAG_DIRECTORY) == 0) 
    {
        rc = -ERROR_NOT_A_DIRECTORY;
        goto exit;
    }

    // inode директории должно иметь права на исполнение
    if (inode_check_perm(inode, process->euid, process->egid, S_IXUSR, S_IXGRP, S_IXOTH) == 0)
    {
        rc = -EACCES;
        goto exit;
    }

    // непосредственно, сама установка домашней директории
    process_set_workdir(process, dentr);

exit:
    file_close(file);
    return rc;
}