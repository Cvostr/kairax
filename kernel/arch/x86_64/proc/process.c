#include "process.h"
#include "memory/paging.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/hh_offset.h"

int last_pid = 0;

process_t*  create_new_process(){
    process_t* process = (process_t*)kmalloc(sizeof(process_t));

    page_table_t* pml4 = new_page_table();
    //...

    process->pml4 = pml4;   //should be physical
    process->brk = 0x0;
    process->pid = last_pid++;

    return process;
}

uintptr_t process_brk(process_t* process, void* addr){
    while((uint64_t)addr > process->brk){
        map_page_mem(process->pml4, process->brk, pmm_alloc_page(), PAGE_USER_ACCESSIBLE | PAGE_WRITABLE | PAGE_PRESENT);
        process->brk += PAGE_SIZE;
    }
    return process->brk;
}