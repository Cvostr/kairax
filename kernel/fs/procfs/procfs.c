#include "procfs.h"
#include "proc/process_list.h"
#include "kairax/kstdlib.h"
#include "string.h"
#include "proc/process.h"

#define PROCFS_ROOTINODE 2

struct file_operations procfs_root_dir_ops = {
    .readdir = procfs_root_file_readdir
};

struct file_operations procfs_process_dir_ops = {
    .readdir = procfs_process_file_readdir
};

struct super_operations procfs_sb_ops = {
    .read_inode = procfs_read_node,
    .find_dentry = procfs_find_dentry
};

struct procfs_procdir_dentry {
    char                    *name;
    int                     dentry_type;
    struct inode_operations *iops;
    struct file_operations  *fops;
    mode_t                  inode_mode;
};

struct file_operations cmdline_fops = {
    .read = procfs_cmdline_read
};
struct file_operations status_fops = {
    .read = procfs_status_read
};
struct file_operations maps_fops = {
    .read = procfs_maps_read
};
struct inode_operations cwd_iops = {
    .readlink = procfs_cwd_readlink
};

#define ALL_READ (S_IRGRP | S_IRUSR | S_IROTH)
#define PROCDIR_INODEMODE   (0555 | INODE_TYPE_DIRECTORY)

static const struct procfs_procdir_dentry procfs_dentries[] = {
    {.name = "cmdline", .dentry_type = DT_REG,  .inode_mode = ALL_READ | INODE_TYPE_FILE, .fops = &cmdline_fops},
    {.name = "status",  .dentry_type = DT_REG,  .inode_mode = ALL_READ | INODE_TYPE_FILE, .fops = &status_fops},
    {.name = "maps",    .dentry_type = DT_REG,  .inode_mode = ALL_READ | INODE_TYPE_FILE, .fops = &maps_fops},
    {.name = "cwd",     .dentry_type = DT_LNK,  .inode_mode = ALL_READ | INODE_FLAG_SYMLINK, .iops = &cwd_iops}
};
#define NPROCDENTRIES (sizeof(procfs_dentries) / sizeof(struct procfs_procdir_dentry)) 

struct file_operations mounts_fops = {
    .read = procfs_mounts_read
};
struct file_operations modules_fops = {
    .read = procfs_modules_read
};
struct file_operations kernel_cmdline_fops = {
    .read = procfs_kernel_cmdline_read
};
static const struct procfs_procdir_dentry procfs_root_dentries[] = {
    {.name = "mounts", .dentry_type = DT_REG,  .inode_mode = ALL_READ | INODE_TYPE_FILE, .fops = &mounts_fops},
    {.name = "modules", .dentry_type = DT_REG,  .inode_mode = ALL_READ | INODE_TYPE_FILE, .fops = &modules_fops},
    {.name = "cmdline", .dentry_type = DT_REG,  .inode_mode = ALL_READ | INODE_TYPE_FILE, .fops = &kernel_cmdline_fops}
};
#define NROOTENTRIES (sizeof(procfs_root_dentries) / sizeof(struct procfs_procdir_dentry)) 

void procfs_init()
{
    filesystem_t* procfs = new_filesystem();
    procfs->name = "procfs";
    procfs->mount = procfs_mount;
    procfs->unmount = procfs_unmount;

    filesystem_register(procfs);
}

ino_t procfs_makeino(pid_t pid, int file)
{
    ino_t res = (pid + 1);
    res <<= 32;
    res |= file;
    return res;
}

void procfs_decodeino(ino_t inode_num, pid_t *pid, int *fileid)
{
    *pid = (inode_num >> 32) - 1;
    *fileid = inode_num & 0xFFFFFFFF;
}

struct inode* procfs_makeinode(struct superblock* sb, ino_t inode)
{
    struct inode* result = NULL;
    struct process *proc = NULL;

    if (inode == PROCFS_ROOTINODE)
    {
        // это корневая директория
        result = new_vfs_inode();
        result->inode = 2;
        result->sb = sb;
        result->uid = 0;
        result->gid = 0;
        result->size = 0;
        result->mode = INODE_TYPE_DIRECTORY | 0777;
        // ее операции
        result->file_ops = &procfs_root_dir_ops;
    } 
    else if (inode > PROCFS_ROOTINODE && inode <= PROCFS_ROOTINODE + NROOTENTRIES)
    {
        // это файл в корневой директории
        const struct procfs_procdir_dentry *procfs_dentry = &procfs_root_dentries[inode - PROCFS_ROOTINODE - 1];
        result = new_vfs_inode();
        result->inode = inode;
        result->sb = sb;
        result->uid = 0;
        result->gid = 0;
        result->size = 0;
        result->mode = procfs_dentry->inode_mode;
        // ее операции
        result->operations = procfs_dentry->iops;
        result->file_ops = procfs_dentry->fops;
    }
    else 
    {
        pid_t pid = 0;
        int fileid = 0;
        procfs_decodeino(inode, &pid, &fileid);

        // пробуем найти процесс по PID с увеличением счетчика ссылок
        proc = process_get_by_id(pid);
        if (proc == NULL)
            return NULL;

        if (fileid == 0)
        {
            // это директория процесса
            result = new_vfs_inode();
            result->inode = inode;
            result->sb = sb;
            result->uid = 0;
            result->gid = 0;
            result->size = 0;
            result->mode = PROCDIR_INODEMODE;
            // ее операции
            result->file_ops = &procfs_process_dir_ops;
        }
        else
        {
            // это файл внутри директории процесса
            fileid --;
            if (fileid >= NPROCDENTRIES)
                goto exit;

            const struct procfs_procdir_dentry *procfs_dentry = &procfs_dentries[fileid];

            // это директория процесса
            result = new_vfs_inode();
            result->inode = inode;
            result->sb = sb;
            result->uid = 0;
            result->gid = 0;
            result->size = 0;
            result->mode = procfs_dentry->inode_mode;
            // ее операции
            result->operations = procfs_dentry->iops;
            result->file_ops = procfs_dentry->fops;
        }
    }

exit:
    if (proc)
        free_process(proc);
    return result;
}

struct inode* procfs_mount(drive_partition_t* drive, struct superblock* sb)
{
    // создаем корневую inode
    struct inode* result = procfs_makeinode(sb, PROCFS_ROOTINODE);

    sb->fs_info = NULL;
    sb->blocksize = 0;
    sb->operations = &procfs_sb_ops;

    return result;
}

int procfs_unmount(struct superblock* sb)
{

}

struct dirent* procfs_root_file_readdir(struct file* dir, uint32_t index)
{
    struct dirent* result = NULL;
    pid_t pid = 0;
    uint32_t cur = 0;
    struct process* proc = NULL;

    size_t processes_total, threads_total;
    get_process_count(&processes_total, &threads_total);

    if (index >= processes_total)
    {
        // индекс больше числа процессов
        // значит это могут быть дополнительные файлы в корневой директории
        size_t file_idx = index - processes_total;

        if (file_idx < NROOTENTRIES)
        {
            const struct procfs_procdir_dentry *dentry = &procfs_root_dentries[file_idx];

            result = new_vfs_dirent();
            result->inode = PROCFS_ROOTINODE + 1 + file_idx;
            result->type = dentry->dentry_type;
            strcpy(result->name, dentry->name);
            return result;
        }

        return NULL;
    }

    // Пропустим сначала все процессы по смещению
    // надо пропустить index процессов
    while (cur < index)
    {
        // пробуем найти процесс по PID с увеличением счетчика ссылок
        proc = process_get_by_id(pid ++);

        if ((proc != NULL) && (proc->type == OBJECT_TYPE_PROCESS))
            cur++;

        // сразу же уменьшаем ссылки
        free_process(proc);
    }

    // теперь ищем первый встречный процесс
    proc = process_get_by_id(pid);
    while ((proc == NULL || proc->type != OBJECT_TYPE_PROCESS) && pid < MAX_PROCESSES) 
    {
        free_process(proc);
        proc = process_get_by_id(++pid);
    }

    // Сюда попадаем, когда обработали все возможные процессы и их больше не осталось
    if (proc == NULL)
    {
        return NULL;
    }

    result = new_vfs_dirent();
    result->inode = procfs_makeino(pid, 0);
    result->type = DT_DIR;
    lltoa(pid, result->name, 10);

    free_process(proc);
    return result;
}

struct dirent* procfs_process_file_readdir(struct file* dir, uint32_t index)
{
    struct dirent* result = NULL;
    struct inode* vfs_inode = dir->inode;

    pid_t pid = 0;
    int fileid = 0;
    procfs_decodeino(vfs_inode->inode, &pid, &fileid);

    struct process *proc = process_get_by_id(pid);
    if (proc == NULL)
        return NULL;

    if (index >= NPROCDENTRIES)
        goto exit;

    const struct procfs_procdir_dentry *procfs_dentry = &procfs_dentries[index];
    
    result = new_vfs_dirent();
    result->inode = procfs_makeino(pid, index + 1); // прибавляем 1, так как 0 отдан под саму inode процесса
    result->type = procfs_dentry->dentry_type;
    strcpy(result->name, procfs_dentry->name);

exit:
    free_process(proc);
    return result;
}

uint64_t procfs_find_dentry(struct superblock* sb, struct inode* vfs_parent_inode, const char *name, int* type)
{
    //printk("procfs: find dentry %s\n", name);
    int i;
    ino_t parent_inode_num = vfs_parent_inode->inode;

    if (parent_inode_num == PROCFS_ROOTINODE)
    {
        // Это корневая директория
        char *endptr;
        pid_t pid = strtol(name, &endptr, 10);
        
        if ((endptr == name) || *endptr != '\0') 
        {
            // это не число, вероятно это один из корневых файлов
            for (i = 0; i < NROOTENTRIES; i ++)
            {
                const struct procfs_procdir_dentry *procfs_dentry = &procfs_root_dentries[i];
                if (strcmp(name, procfs_dentry->name) == 0)
                {
                    *type = procfs_dentry->dentry_type;
                    return PROCFS_ROOTINODE + 1 + i;
                }
            }
        }
        else
        {
            // если это число, то, это скорее всего директория с процессом
            // пробуем получить процесс с увеличением счетчика ссылок
            struct process *proc = process_get_by_id(pid);
            // процесс должен существовать и не быть потоком
            if (proc == NULL || proc->type != OBJECT_TYPE_PROCESS)
            {
                free_process(proc);
                return WRONG_INODE_INDEX;
            }

            *type = DT_DIR;
            free_process(proc);
            return procfs_makeino(pid, 0);
        }
    }
    else
    {
        // Это директория процесса
        pid_t pid = 0;
        int fileid = 0;
        procfs_decodeino(parent_inode_num, &pid, &fileid);

        if (pid >= 0 && fileid == 0)     
        {
            for (i = 0; i < NPROCDENTRIES; i ++)
            {
                const struct procfs_procdir_dentry *procfs_dentry = &procfs_dentries[i];
                if (strcmp(name, procfs_dentry->name) == 0)
                {
                    *type = procfs_dentry->dentry_type;
                    return procfs_makeino(pid, i + 1);
                }
            }
        }   
    }

    return WRONG_INODE_INDEX;
}

struct inode* procfs_read_node(struct superblock* sb, ino_t ino_num)
{
    return procfs_makeinode(sb, ino_num);
}
