#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "errors.h"
#include "proc/process.h"
#include "cpu/cpu_local_x64.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "mem/pmm.h"
#include "kstdlib.h"
#include "string.h"

void* sys_memory_map(void* address, uint64_t length, int protection, int flags, int fd, int offset)
{
    //printf("ADDR %s ", ulltoa(address, 16));
    //printf_stdout("SZ %s\n", ulltoa(length, 16));
    struct process* process = cpu_get_current_thread()->process;

    if (length == 0) {
        return (void*)-ERROR_INVALID_VALUE;
    }

    if (address + length >= USERSPACE_MAX_ADDR) {
        return (void*)-ERROR_INVALID_VALUE;
    }

    length = align(length, PAGE_SIZE);

    // Получить предпочитаемый адрес
    address = process_get_free_addr(process, length, align_down((uint64_t) address, PAGE_SIZE));

    // Сформировать регион
    struct mmap_range* range = kmalloc(sizeof(struct mmap_range));
    if (range == NULL) {
        return -ENOMEM;
    }
    
    range->base = (uint64_t) address;
    range->length = length;
    range->protection = protection | PAGE_PROTECTION_USER;

    process_add_mmap_region(process, range);

    //NOT IMPLEMENTED FOR FILE MAP
    //TODO: IMPLEMENT

    //printf_stdout(" FOUND ADDR %s\n", ulltoa(address, 16));
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

    // Освободить память
    vm_table_unmap_region(process->vmemory_table, address, length);

    kfree(region);

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