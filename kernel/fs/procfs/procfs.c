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
    char                   *name;
    int                     dentry_type;
    struct file_operations *fops;
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

#define ALL_READ (S_IRGRP | S_IRUSR | S_IROTH)
#define PROCDIR_INODEMODE   (0555 | INODE_TYPE_DIRECTORY)

static const struct procfs_procdir_dentry procfs_dentries[] = {
    {.name = "cmdline", .dentry_type = DT_REG,  .inode_mode = ALL_READ | INODE_TYPE_FILE, .fops = &cmdline_fops},
    {.name = "status",  .dentry_type = DT_REG,  .inode_mode = ALL_READ | INODE_TYPE_FILE, .fops = &status_fops},
    {.name = "maps",    .dentry_type = DT_REG,  .inode_mode = ALL_READ | INODE_TYPE_FILE, .fops = &maps_fops}
};
#define NPROCDENTRIES (sizeof(procfs_dentries) / sizeof(struct procfs_procdir_dentry)) 

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
    else 
    {
        pid_t pid = 0;
        int fileid = 0;
        procfs_decodeino(inode, &pid, &fileid);
        //printk("procfs: procfs_makeinode: pid %i fileid %i\n", pid, fileid);

        struct process *proc = process_get_by_id(pid);
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
                return NULL;

            struct procfs_procdir_dentry *procfs_dentry = &procfs_dentries[fileid];

            // это директория процесса
            result = new_vfs_inode();
            result->inode = inode;
            result->sb = sb;
            result->uid = 0;
            result->gid = 0;
            result->size = 0;
            result->mode = procfs_dentry->inode_mode;
            // ее операции
            result->file_ops = procfs_dentry->fops;
        }
    }

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

    // Пропустим сначала все процессы по смещению
    // надо пропустить index процессов
    while (cur < index)
    {
        proc = process_get_by_id(pid ++);

        if ((proc != NULL) && (proc->type == OBJECT_TYPE_PROCESS))
            cur++;
    }

    proc = process_get_by_id(pid);
    while ((proc == NULL || proc->type != OBJECT_TYPE_PROCESS) && pid < MAX_PROCESSES) 
    {
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
        return NULL;

    struct procfs_procdir_dentry *procfs_dentry = &procfs_dentries[index];
    
    result = new_vfs_dirent();
    result->inode = procfs_makeino(pid, index + 1); // прибавляем 1, так как 0 отдан под саму inode процесса
    result->type = procfs_dentry->dentry_type;
    strcpy(result->name, procfs_dentry->name);

    return result;
}

uint64_t procfs_find_dentry(struct superblock* sb, struct inode* vfs_parent_inode, const char *name, int* type)
{
    //printk("procfs: find dentry %s\n", name);
    ino_t parent_inode_num = vfs_parent_inode->inode;

    if (parent_inode_num == PROCFS_ROOTINODE)
    {
        // Это корневая директория
        char *endptr;
        pid_t pid = strtol(name, &endptr, 10);
        
        if (endptr == name) 
        {
            // это не число
        }
        else
        {
            // если это число, то, это скорее всего директория с процессом
            // пробуем получить процесс
            struct process *proc = process_get_by_id(pid);
            // процесс должен существовать и не быть потоком
            if (proc == NULL || proc->type != OBJECT_TYPE_PROCESS)
                return WRONG_INODE_INDEX;

            *type = DT_DIR;
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
            for (int i = 0; i < NPROCDENTRIES; i ++)
            {
                struct procfs_procdir_dentry *procfs_dentry = &procfs_dentries[i];
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
