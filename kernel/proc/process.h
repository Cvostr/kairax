#ifndef _PROCESS_H
#define _PROCESS_H

#include "list/list.h"
#include "fs/vfs/file.h"
#include "fs/vfs/stat.h"

#define MAX_DESCRIPTORS     48
#define MAX_PATH_LEN 512

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
    void*           vmemory_table;  
    // Связный список потоков
    list_t*         threads;  
    // Указатели на открытые файловые дескрипторы
    file_t*         fds[MAX_DESCRIPTORS];

    spinlock_t      fd_spinlock;
} process_t;

//Создать новый пустой процесс
process_t*  create_new_process();

int create_new_process_from_image(char* image);

// Установить адрес конца памяти процесса
uintptr_t        process_brk_flags(process_t* process, void* addr, uint64_t flags);

uintptr_t        process_brk(process_t* process, void* addr);

int process_alloc_memory(process_t* process, uintptr_t start, uintptr_t size, uint64_t flags);

int process_open_file(process_t* process, const char* path, int mode, int flags);

int process_close_file(process_t* process, int fd);

size_t process_read_file(process_t* process, int fd, char* buffer, size_t size);

int process_stat(process_t* process, int fd, struct stat* stat);

int process_readdir(process_t* process, int fd, struct dirent* dirent);

int process_get_working_dir(process_t* process, char* buffer, size_t size);

int process_set_working_dir(process_t* process, const char* buffer);

#endif