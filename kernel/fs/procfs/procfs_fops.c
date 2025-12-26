#include "procfs.h"
#include "proc/process_list.h"
#include "kairax/kstdlib.h"
#include "kairax/stdio.h"
#include "string.h"
#include "proc/process.h"
#include "mem/vm_area.h"
#include "mem/kheap.h"
#include "mod/module_stor.h"

size_t procfs_read_string(const char *str, size_t len, char *buffer, size_t count, loff_t offset)
{
    size_t to_read = MIN(len - offset, count);
    memcpy(buffer, str + offset, to_read);
    return to_read;
}

// для cmdline
ssize_t procfs_cmdline_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    size_t readed = 0;
    ino_t inode_num = file->inode->inode;
    pid_t pid = 0;
    int fileid = 0;
    procfs_decodeino(inode_num, &pid, &fileid);

    // пробуем найти процесс по PID с увеличением счетчика ссылок
    struct process *proc = process_get_by_id(pid);
    if (proc == NULL)
        return -ENOENT;
    
    if (proc->type != OBJECT_TYPE_PROCESS)
    {
        readed = -EINVAL;
        goto exit;
    }

    readed = procfs_read_string(proc->name, strlen(proc->name), buffer, count, offset);
    file->pos += readed;

exit:
    free_process(proc);
    return readed;
}

// для status
ssize_t procfs_status_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    size_t readed = 0;
    ino_t inode_num = file->inode->inode;
    pid_t pid = 0;
    int fileid = 0;
    procfs_decodeino(inode_num, &pid, &fileid);

    // пробуем найти процесс по PID с увеличением счетчика ссылок
    struct process *proc = process_get_by_id(pid);
    if (proc == NULL)
        return -ENOENT;
    
    if (proc->type != OBJECT_TYPE_PROCESS)
    {
        readed = -EINVAL;
        goto exit;
    }

    pid_t ppid = 0;
    if (proc->parent)
        ppid = proc->parent->pid;

    size_t thread_count = list_size(proc->threads);
    
    char statbuffer[500];

    int written = sprintf(statbuffer, sizeof(statbuffer), 
        "Name: %s\nUmask: %i\nState: %i\nPid: %i\nPPid: %i\nUid: %i %i\nGid: %i %i\nThreads: %i",
        proc->name, 
        proc->umask,
        proc->state,
        proc->pid,
        ppid,
        proc->uid, proc->euid,
        proc->gid, proc->egid,
        thread_count
    );

    readed = procfs_read_string(statbuffer, written, buffer, count, offset);
    file->pos += readed;
    
exit:
    free_process(proc);
    return readed;
}

int procfs_fill_mapping_str(struct mmap_range *region, char *out, size_t len)
{
    // по умолчанию всегда читаемо и приватно
    char protection[] = "r--p";

    int prot = region->protection;
    int flags = region->flags;
    //if (prot & PAGE_PROTECTION_READ_ENABLE)
    //    protection[0] = 'r';
    if (prot & PAGE_PROTECTION_WRITE_ENABLE)
        protection[1] = 'w';
    if (prot & PAGE_PROTECTION_EXEC_ENABLE)
        protection[2] = 'x';
    if (flags & MAP_SHARED)
        protection[3] = 's';

    return sprintf(out, len, "%p-%p %s %i\n", 
        region->base, region->base + region->length, protection, region->file_offset);
}

ssize_t procfs_maps_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    ssize_t readed = 0;
    ino_t inode_num = file->inode->inode;
    pid_t pid = 0;
    int fileid = 0;
    procfs_decodeino(inode_num, &pid, &fileid);

    // пробуем найти процесс по PID с увеличением счетчика ссылок
    struct process *proc = process_get_by_id(pid);
    if (proc == NULL)
        return -ENOENT;
    
    if (proc->type != OBJECT_TYPE_PROCESS)
    {
        readed = -EINVAL;
        goto exit;
    }

    // временный буфер для строки мапинга
    char mapbuffer[200];

    if (proc->mmap_ranges == NULL)
        goto exit;

    size_t mmaps_total = list_size(proc->mmap_ranges);
    if (mmaps_total == 0)
        goto exit;

    size_t map_i = 0;
    // Счетчик того, сколько осталось байт до offset
    size_t cntr = offset;
    // Сколько надо считать из mapbuffer
    size_t remain_from_last = 0;
    size_t offset_from_last = 0;
    while (cntr > 0)
    {
        struct mmap_range *region = list_get(proc->mmap_ranges, map_i ++);
        if (region == NULL)
            goto exit;
        int curlen = procfs_fill_mapping_str(region, mapbuffer, sizeof(mapbuffer));
        size_t decrement = MIN(curlen, cntr); 
        cntr -= decrement;

        remain_from_last = curlen - decrement;
        offset_from_last = decrement;
    }

    // Если остался несчитанный с того раза кусок строки - надо считать 
    if (remain_from_last > 0)
    {
        readed += procfs_read_string(mapbuffer + offset_from_last, remain_from_last, buffer, count, 0);
    }

    while (readed < count)
    {
        struct mmap_range *region = list_get(proc->mmap_ranges, map_i ++);
        if (region == NULL)
            goto exit;

        // сформировать строку - описание
        int curlen = procfs_fill_mapping_str(region, mapbuffer, sizeof(mapbuffer));

        // записать строку
        readed += procfs_read_string(mapbuffer, curlen, buffer + readed, count - readed, 0);
    }

exit:
    file->pos += readed;
    free_process(proc);
    return readed;
}

ssize_t procfs_cwd_readlink(struct inode* inode, char* pathbuf, size_t pathbuflen)
{
    ssize_t result = 0;
    size_t required_size = 0;
    ino_t inode_num = inode->inode;
    pid_t pid = 0;
    int fileid = 0;
    procfs_decodeino(inode_num, &pid, &fileid);

    struct process *proc = process_get_by_id(pid);
    if (proc == NULL)
        return -ENOENT;
    
    if (proc->type != OBJECT_TYPE_PROCESS)
    {
        result = -EINVAL;
        goto exit;
    }

    acquire_spinlock(&proc->pwd_lock);
    if (proc->pwd) 
    {    
        // Вычисляем необходимый размер буфера
        vfs_dentry_get_absolute_path(proc->pwd, &required_size, NULL);
        
        char *tmpbuf = kmalloc(required_size + 1);

        // Записываем путь в буфер
        vfs_dentry_get_absolute_path(proc->pwd, NULL, tmpbuf);

        result = procfs_read_string(tmpbuf, required_size, pathbuf, pathbuflen, 0);

        kfree(tmpbuf);
    }
    release_spinlock(&proc->pwd_lock);

exit:
    free_process(proc);
    return result;
}

int procfs_fill_mount_str(struct superblock* sb, char *out, size_t len)
{
    size_t reqd_size = 0;
    char* abs_path_buffer = NULL;
    vfs_dentry_get_absolute_path(sb->root_dir, &reqd_size, NULL);
    abs_path_buffer = kmalloc(reqd_size + 1);
    memset(abs_path_buffer, 0, reqd_size + 1);
    vfs_dentry_get_absolute_path(sb->root_dir, NULL, abs_path_buffer);

    int res = sprintf(out, len, "%s %s %s\n", 
        sb->partition ? sb->partition->name : "none",
        abs_path_buffer,
        sb->filesystem->name
    );

    kfree(abs_path_buffer);

    return res;
}

ssize_t procfs_mounts_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    size_t mount_i = 0;
    ssize_t readed = 0;
    struct superblock* sb = NULL;
    // временный буфер для строки
    char mapbuffer[300];

    // Счетчик того, сколько осталось байт до offset
    size_t cntr = offset;
    // Сколько надо считать из mapbuffer
    size_t remain_from_last = 0;
    size_t offset_from_last = 0;
    while (cntr > 0)
    {
        sb = vfs_get_mounted_sb(mount_i ++);
        if (sb == NULL)
            return 0;

        int curlen = procfs_fill_mount_str(sb, mapbuffer, sizeof(mapbuffer));
        size_t decrement = MIN(curlen, cntr); 
        cntr -= decrement;

        remain_from_last = curlen - decrement;
        offset_from_last = decrement;
    }

    // Если остался несчитанный с того раза кусок строки - надо считать 
    if (remain_from_last > 0)
    {
        readed += procfs_read_string(mapbuffer + offset_from_last, remain_from_last, buffer, count, 0);
    }

    while (readed < count)
    {
        sb = vfs_get_mounted_sb(mount_i ++);
        if (sb == NULL)
            goto exit;

        // сформировать строку - описание
        int curlen = procfs_fill_mount_str(sb, mapbuffer, sizeof(mapbuffer));

        // записать строку
        readed += procfs_read_string(mapbuffer, curlen, buffer + readed, count - readed, 0);
    }

exit:
    file->pos += readed;
    return readed;
}

int procfs_fill_module_str(struct module* mod, char *out, size_t len)
{
    int res = sprintf(out, len, "%s %i\n", 
        mod->name, mod->size
    );

    return res;
}

ssize_t procfs_modules_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    size_t mod_i = 0;
    ssize_t readed = 0;
    struct module* mod = NULL;
    // временный буфер для строки
    char mapbuffer[300];

    // Счетчик того, сколько осталось байт до offset
    size_t cntr = offset;
    // Сколько надо считать из mapbuffer
    size_t remain_from_last = 0;
    size_t offset_from_last = 0;
    while (cntr > 0)
    {
        mod = mstor_get_module(mod_i ++);
        if (mod == NULL)
            return 0;

        int curlen = procfs_fill_module_str(mod, mapbuffer, sizeof(mapbuffer));
        size_t decrement = MIN(curlen, cntr); 
        cntr -= decrement;

        remain_from_last = curlen - decrement;
        offset_from_last = decrement;
    }

    // Если остался несчитанный с того раза кусок строки - надо считать 
    if (remain_from_last > 0)
    {
        readed += procfs_read_string(mapbuffer + offset_from_last, remain_from_last, buffer, count, 0);
    }

    while (readed < count)
    {
        mod = mstor_get_module(mod_i ++);
        if (mod == NULL)
            goto exit;

        // сформировать строку - описание
        int curlen = procfs_fill_module_str(mod, mapbuffer, sizeof(mapbuffer));

        // записать строку
        readed += procfs_read_string(mapbuffer, curlen, buffer + readed, count - readed, 0);
    }

exit:
    file->pos += readed;
    return readed;
}