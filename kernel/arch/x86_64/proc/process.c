#include "process.h"
#include "memory/paging.h"
#include "memory/pmm.h"
#include "string.h"
#include "memory/hh_offset.h"

int last_pid = 0;

process_t*  create_new_process(){
    process_t* process = (process_t*)alloc_page();

    page_table_t* pml4 = new_page_table();
    //memcpy(pml4, get_kernel_pml4(), 4096);
    for(uintptr_t i = 0; i <= 1024 * 1024 * 32; i += 4096){
        uint64_t npageFlags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL;
        map_page_mem(pml4, i, i, npageFlags);
        map_page_mem(pml4, P2V(i), i, npageFlags);
    }

    process->pml4 = pml4;   //should be physical
    process->brk = 0x0;
    process->pid = last_pid++;

    return process;
}

uintptr_t process_brk(process_t* process, void* addr){
    while((uint64_t)addr > process->brk){
        map_page_mem(process->pml4, process->brk, alloc_page(), PAGE_USER_ACCESSIBLE | PAGE_WRITABLE | PAGE_PRESENT);
        process->brk += PAGE_SIZE;
    }
    return process->brk;
}