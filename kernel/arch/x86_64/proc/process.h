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

//Создать новый пустой процесс
process_t*  create_new_process();

int create_new_process_from_image(char* image);

// Установить адрес конца памяти процесса
uintptr_t        process_brk(process_t* process, void* addr);

#endif