#ifndef _PROCESS_H
#define _PROCESS_H

#include "list/list.h"
#include "fs/vfs/file.h"
#include "fs/vfs/stat.h"
#include "mem/paging.h"

#define MAX_DESCRIPTORS         64
#define PROCESS_MAX_ARGS        65535
#define PROCESS_MAX_ARGS_SIZE   (128ULL * 1024 * 1024)

struct mmap_range {
    uint64_t        base;
    uint64_t        length;
    int             protection;
};

struct process {
    char                name[30];
    // ID процесса
    pid_t               pid;
    // Адрес конца адресного пространства процесса
    uint64_t            brk;
    uint64_t            threads_stack_top;

    uint32_t            state;
    // Рабочая папка
    struct file*        workdir;
    // Таблица виртуальной памяти процесса
    struct vm_table*    vmemory_table;  
    // Процесс - родитель
    struct process*     parent;
    // Связный список потоков
    list_t*             threads;  
    // Связный список потомков
    list_t*             children;
    // Указатели на открытые файловые дескрипторы
    struct file*        fds[MAX_DESCRIPTORS];
    spinlock_t          fd_lock;
    // начальные данные для TLS
    char*               tls;
    size_t              tls_size;

    list_t*             mmap_ranges;
    spinlock_t          mmap_lock;
};

struct process_create_info {
    char*   current_directory;
    int  num_args;
    char**  args;
};

//Создать новый пустой процесс
struct process*  create_new_process(struct process* parent);

int process_get_relative_direntry(struct process* process, int dirfd, const char* path, struct dentry** result);

void free_process(struct process* process);

int create_new_process_from_image(struct process* parent, char* image, struct process_create_info* info);

// Установить адрес конца памяти процесса
uintptr_t        process_brk_flags(struct process* process, void* addr, uint64_t flags);

uintptr_t        process_brk(struct process* process, uint64_t addr);

int process_alloc_memory(struct process* process, uintptr_t start, uintptr_t size, uint64_t flags);

void* process_alloc_stack_memory(struct process* process, size_t stack_size);

struct file* process_get_file(struct process* process, int fd);

int process_load_arguments(struct process* process, int argc, char** argv, char** args_mem);

int process_add_file(struct process* process, struct file* file);

void process_add_mmap_region(struct process* process, struct mmap_range* region);

struct mmap_range* process_get_range_by_addr(struct process* process, uint64_t addr);

int process_handle_page_fault(struct process* process, uint64_t address);

int process_create_thread(struct process* process, void* entry_ptr, void* arg, pid_t* tid, size_t stack_size);

#endif