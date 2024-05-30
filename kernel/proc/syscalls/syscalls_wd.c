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
    int rc = 0;
    struct process* process = cpu_get_current_thread()->process;
    struct dentry* workdir_dentry = process->pwd != NULL ? process->pwd : NULL;
    struct dentry* new_workdir = vfs_dentry_traverse_path(workdir_dentry, buffer);

    if (new_workdir) {
        // Проверить тип (должна быть директория)
        if ((new_workdir->flags & DENTRY_TYPE_DIRECTORY) == DENTRY_TYPE_DIRECTORY) {

            acquire_spinlock(&process->pwd_lock);
            //  Закрываем старый файл, если был
            if (process->pwd) {
                dentry_close(process->pwd);
            }

            // Увеличиваем счетчик ссылок и сохраняем
            dentry_open(new_workdir);
            process->pwd = new_workdir;

            release_spinlock(&process->pwd_lock);
        } else {
            // Это не директория
            rc = -ERROR_NOT_A_DIRECTORY;
        }
    } else {
        rc = -ERROR_NO_FILE;
    }

    return rc;
}