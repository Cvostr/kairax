#ifndef _PROCESS_H
#define _PROCESS_H

#include "list/list.h"
#include "fs/vfs/file.h"
#include "fs/vfs/stat.h"

#define MAX_DESCRIPTORS     48
#define MAX_PATH_LEN 512

typedef struct {
    char            name[30];
    // ID процесса
    pid_t           pid;
    // Адрес конца адресного пространства процесса
    uint64_t        brk;

    uint32_t        state;
    // Путь к текущей рабочей папке
    char            cur_dir[MAX_PATH_LEN];
    // Таблица виртуальной памяти процесса
    void*           vmemory_table;  
    // Связный список потоков
    list_t*         threads;  
    // Связный список потомков
    list_t*         children;
    // Указатели на открытые файловые дескрипторы
    file_t*         fds[MAX_DESCRIPTORS];
    // начальные данные для TLS
    char*           tls;
    size_t          tls_size;

    spinlock_t      fd_spinlock;
} process_t;

//Создать новый пустой процесс
process_t*  create_new_process();

void free_process(process_t* process);

int create_new_process_from_image(char* image);

// Установить адрес конца памяти процесса
uintptr_t        process_brk_flags(process_t* process, void* addr, uint64_t flags);

uintptr_t        process_brk(process_t* process, uint64_t addr);

int process_alloc_memory(process_t* process, uintptr_t start, uintptr_t size, uint64_t flags);

int process_open_file(process_t* process, const char* path, int mode, int flags);

int process_close_file(process_t* process, int fd);

ssize_t process_read_file(process_t* process, int fd, char* buffer, size_t size);

off_t process_file_seek(process_t* process, int fd, off_t offset, int whence); 

int process_stat(process_t* process, int fd, struct stat* stat);

int process_readdir(process_t* process, int fd, struct dirent* dirent);

int process_get_working_dir(process_t* process, char* buffer, size_t size);

int process_set_working_dir(process_t* process, const char* buffer);

int process_create_thread(process_t* process, void* entry_ptr, void* arg, pid_t* tid, size_t stack_size);

#endif