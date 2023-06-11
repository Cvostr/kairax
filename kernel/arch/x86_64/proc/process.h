#ifndef _PROCESS_H
#define _PROCESS_H

#include "memory/paging.h"
#include "list/list.h"

typedef struct PACKED {
    char    name[30];

    uint64_t pid;
    uint64_t brk;

    uint32_t state;
    char cur_dir[30];

    page_table_t*   pml4;  
    list_t*         threads;  
} process_t;

//Создать новый пустой процесс
process_t*  create_new_process();

int create_new_process_from_image(char* image);

// Установить адрес конца памяти процесса
uintptr_t        process_brk_flags(process_t* process, void* addr, uint64_t flags);

uintptr_t        process_brk(process_t* process, void* addr);

int process_alloc_memory(process_t* process, uintptr_t start, uintptr_t size, uint64_t flags);

#endif