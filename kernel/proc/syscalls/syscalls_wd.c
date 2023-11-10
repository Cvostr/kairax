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
    struct process* process = cpu_get_current_thread()->process;

    if (process->workdir) {
        
        // Вычисляем необходимый размер буфера
        vfs_dentry_get_absolute_path(process->workdir->dentry, &reqd_size, NULL);
        
        if (reqd_size + 1 > size) {
            // Размер буфера недостаточный
            rc = -ERROR_RANGE;
            goto exit;
        }

        // Записываем путь в буфер
        vfs_dentry_get_absolute_path(process->workdir->dentry, NULL, buffer);
    }

exit:
    return rc;
}

int sys_set_working_dir(const char* buffer)
{
    int rc = 0;
    struct process* process = cpu_get_current_thread()->process;
    struct dentry* workdir_dentry = process->workdir != NULL ? process->workdir->dentry : NULL;
    struct file* new_workdir = file_open(workdir_dentry, buffer, 0, 0);

    if (new_workdir) {
        // файл открыт, убеждаемся что это директория 
        if (new_workdir->inode->mode & INODE_TYPE_DIRECTORY) {

            //  Закрываем старый файл, если был
            if (process->workdir) {
                file_close(process->workdir);
            }

            process->workdir = new_workdir;
        } else {
            // Это не директория
            rc = -ERROR_NOT_A_DIRECTORY;
            file_close(new_workdir);
        }
    } else {
        rc = -ERROR_NO_FILE;
    }

    return rc;
}