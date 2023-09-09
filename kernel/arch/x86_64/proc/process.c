#include "memory/kernel_vmm.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/mem_layout.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "proc/thread_scheduler.h"
#include "fs/vfs/vfs.h"
#include "kstdlib.h"

uintptr_t process_brk_flags(struct process* process, void* addr, uint64_t flags)
{
    uintptr_t uaddr = (uintptr_t)addr;
    //Выравнивание до размера страницы в большую сторону
    uaddr += (PAGE_SIZE - (uaddr % PAGE_SIZE));
    //До тех пор, пока адрес конца памяти процесса меньше, выделяем страницы
    while ((uint64_t)uaddr > process->brk) {
        map_page_mem(process->vmemory_table->arch_table, process->brk, (physical_addr_t)pmm_alloc_page(), flags);
        process->brk += PAGE_SIZE;
    }

    //До тех пор, пока адрес конца памяти процесса больше, освобождаем страницы
    /*while((uint64_t)uaddr < process->brk) {
        uintptr_t virtual_page_start = process->brk - PAGE_SIZE;
        physical_addr_t phys_addr = get_physical_address(process->vmemory_table, virtual_page_start);
        unmap_page(process->vmemory_table, virtual_page_start);
        pmm_free_page((void*)phys_addr);
        process->brk -= PAGE_SIZE;
    }*/

    return process->brk;
}

uintptr_t process_brk(struct process* process, uint64_t addr)
{
    return process_brk_flags(process, (void*)addr, PAGE_USER_ACCESSIBLE | PAGE_WRITABLE | PAGE_PRESENT);
}