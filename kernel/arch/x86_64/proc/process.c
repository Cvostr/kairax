#include "process.h"
#include "memory/paging.h"
#include "memory/pmm.h"

int last_pid = 0;

process_t*  create_new_process(){
    process_t* process = (process_t*)alloc_page();

    page_table_t* pml4 = (page_table_t*)alloc_page(); 
    process->pml4 = pml4;
    process->pid = last_pid++;

    return process;
}

void process_brk(process_t* process, void* addr){

}