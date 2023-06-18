#ifndef _PROCESS_H
#define _PROCESS_H

#include "memory/paging.h"
#include "list/list.h"
#include "fs/vfs/file.h"

#define MAX_DESCRIPTORS     48

typedef struct PACKED {
    char            name[30];
    // ID процесса
    uint64_t        pid;
    // Адрес конца адресного пространства процесса
    uint64_t        brk;

    uint32_t        state;
    // Путь к текущей рабочей папке
    char            cur_dir[MAX_PATH_LEN];
    // Таблица виртуальной памяти процесса
    page_table_t*   pml4;  
    // Связный список потоков
    list_t*         threads;  
    // Указатели на открытые файловые дескрипторы
    file_t*         fds[MAX_DESCRIPTORS];
} process_t;

//Создать новый пустой процесс
process_t*  create_new_process();

int create_new_process_from_image(char* image);

// Установить адрес конца памяти процесса
uintptr_t        process_brk_flags(process_t* process, void* addr, uint64_t flags);

uintptr_t        process_brk(process_t* process, void* addr);

int process_alloc_memory(process_t* process, uintptr_t start, uintptr_t size, uint64_t flags);

#endif