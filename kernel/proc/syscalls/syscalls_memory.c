#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "errors.h"
#include "proc/process.h"
#include "cpu/cpu_local.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "string.h"

void* sys_memory_map(void* address, uint64_t length, int protection, int flags, int fd, off_t offset)
{
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = NULL;
    int res = 0;

    if (length == 0) {
        return (void*)-ERROR_INVALID_VALUE;
    }

    if (address + length >= USERSPACE_MAX_ADDR) {
        return (void*)-ERROR_INVALID_VALUE;
    }

    // Надо проверить, что задан ровно один из флагов MAP_SHARED и MAP_PRIVATE
    int shared_private_mask = (flags & (MAP_SHARED | MAP_PRIVATE));
    if (shared_private_mask == (MAP_SHARED | MAP_PRIVATE) ||
        shared_private_mask == 0) 
    {
        return -EINVAL;
    }

    if ((flags & MAP_ANONYMOUS) == 0) 
    {
        // offset должен быть выровнен
        if ((offset % PAGE_SIZE) > 0)
        {
            return -EINVAL;
        }

        // Найдем файл по дескриптору
        file = process_get_file(process, fd);  

        if (file == NULL) {
            return (void*)-ERROR_BAD_FD;
        }

        // Проверим тип файла
        mode_t inode_type = file->inode->mode & INODE_TYPE_MASK;
        if (inode_type != INODE_TYPE_FILE &&
            inode_type != INODE_FLAG_CHARDEVICE
        ) 
        {
            return -EACCES;
        }

        // Файл должен быть доступен для чтения
        if (file_allow_read(file) == FALSE)
        {
            return -EACCES;
        }

        // Если есть флаг SHARED и у запрашивается разрешение на запись в регион памяти,
        // то файл должен быть открыт для записи
        if (((flags & MAP_SHARED) == MAP_SHARED) && 
            ((protection & PAGE_PROTECTION_WRITE_ENABLE) == PAGE_PROTECTION_WRITE_ENABLE) &&
            file_allow_write(file) == FALSE
        )
        {
            return -EACCES;
        }

        if (file->ops->mmap == NULL) {
            return -ENODEV;
        }
    }

    length = align(length, PAGE_SIZE);

    // Получить предпочитаемый адрес
    address = process_get_free_addr(process, length, align_down((uint64_t) address, PAGE_SIZE));

    // Сформировать регион
    struct mmap_range* range = new_mmap_region();
    if (range == NULL) {
        return -ENOMEM;
    }
    
    range->base = (uint64_t) address;
    range->length = length;
    range->protection = protection | PAGE_PROTECTION_USER;
    range->flags = flags;

    if (file != NULL) 
    {
        file_acquire(file);
        range->file = file;
        range->file_offset = offset;

        res = file->ops->mmap(file, range, 0, length);
        if (res != 0) 
        {
            kfree(range);
            return (void*) res;
        }
    }

    process_add_mmap_region(process, range);

    return address;
}

int sys_memory_unmap(void* address, uint64_t length)
{
    // ONLY implemented unmapping by address
    // TODO : implement splitting
    struct process* process = cpu_get_current_thread()->process;
    int rc = 0;

    if (length == 0) {
        return -ERROR_INVALID_VALUE;
    }

    acquire_spinlock(&process->mmap_lock);
    struct mmap_range* region = process_get_region_by_addr(process, address);
    
    if (region == NULL) {
        rc = -1; //Уточнить код ошибки
        goto exit;
    }

    // TODO: reimplement
    length = region->length;

    // Удалить регион из списка
    list_remove(process->mmap_ranges, region);

    // Является ли регион разделяемым?
    int shared = (region->flags & MAP_SHARED) == MAP_SHARED;

    // Освободить память
    vm_table_unmap_region(process->vmemory_table, address, length, !shared);

    mmap_region_unref(region);

exit:
    release_spinlock(&process->mmap_lock);
    return rc;
}

int sys_memory_protect(void* address, uint64_t length, int protection)
{
    // ONLY implemented non conditional
    // TODO : implement splitting
    struct process* process = cpu_get_current_thread()->process;

    protection |= PAGE_PROTECTION_USER;

    vm_table_protect_region(process->vmemory_table, address, length, protection);

    // TODO: Реализовать
    return -1;
}