#include "process.h"
#include "memory/paging.h"
#include "memory/pmm.h"

process_t*  create_new_process(){
    process_t* process = (process_t*)alloc_page();

    page_table_t* pml4 = (page_table_t*)alloc_page(); 
}

void process_brk(process_t* process, void* addr){

}