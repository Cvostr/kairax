#include "process.h"
#include "memory/kernel_vmm.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/mem_layout.h"

int last_pid = 0;

process_t*  create_new_process()
{
    process_t* process = (process_t*)kmalloc(sizeof(process_t));

    page_table_t* pml4 = vmm_clone_kernel_memory_map();

    process->pml4 = pml4;   //should be physical
    process->brk = 0x0;
    process->pid = last_pid++;

    return process;
}

uintptr_t process_brk(process_t* process, void* addr)
{
    uintptr_t uaddr = addr;
    //Выравнивание до размера страницы в большую сторону
    uaddr += (PAGE_SIZE - (uaddr % PAGE_SIZE));
    //До тех пор, пока адрес конца памяти процесса меньше, выделяем страницы
    while((uint64_t)uaddr > process->brk){
        map_page_mem(process->pml4, process->brk, pmm_alloc_page(), PAGE_USER_ACCESSIBLE | PAGE_WRITABLE | PAGE_PRESENT);
        process->brk += PAGE_SIZE;
    }
    
    return process->brk;
}