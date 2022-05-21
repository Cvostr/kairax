#ifndef _PROCESS_H
#define _PROCESS_C

#include "memory/paging.h"



typedef struct PACKED {
    char    name[30];

    uint64_t pid;
    uint64_t brk;

    uint32_t state;
    char cur_dir[30];

    page_table_t*    pml4;    
} process_t;

process_t*  create_new_process();

uintptr_t        process_brk(process_t* process, void* addr);

#endif