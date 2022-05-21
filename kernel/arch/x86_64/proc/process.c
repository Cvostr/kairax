#include "process.h"
#include "memory/paging.h"
#include "memory/pmm.h"
#include "string.h"

int last_pid = 0;

process_t*  create_new_process(){
    process_t* process = (process_t*)alloc_page();

    page_table_t* pml4 = new_page_table();

    process->pml4 = pml4;
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