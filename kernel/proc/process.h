#ifndef _PROCESS_H
#define _PROCESS_H

#include "list/list.h"
#include "fs/vfs/file.h"
#include "fs/vfs/stat.h"

#define MAX_DESCRIPTORS     64
#define PROCESS_MAX_ARGS    65535

struct process {
    char            name[30];
    // ID процесса
    pid_t           pid;
    // Адрес конца адресного пространства процесса
    uint64_t        brk;

    uint32_t        state;
    // Путь к текущей рабочей папке
    struct dentry*  cwd_dentry;
    struct inode*   cwd_inode;
    // Таблица виртуальной памяти процесса
    void*           vmemory_table;  
    // Процесс - родитель
    struct process*      parent;
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
};

struct process_create_info {
    char*   current_directory;
    size_t  num_args;
    char**  args;
};

//Создать новый пустой процесс
struct process*  create_new_process(struct process* parent);

void free_process(struct process* process);

int create_new_process_from_image(struct process* parent, char* image, struct process_create_info* info);

// Установить адрес конца памяти процесса
uintptr_t        process_brk_flags(struct process* process, void* addr, uint64_t flags);

uintptr_t        process_brk(struct process* process, uint64_t addr);

int process_create_process(struct process* process, const char* filepath, struct process_create_info* info);

int process_alloc_memory(struct process* process, uintptr_t start, uintptr_t size, uint64_t flags);

file_t* process_get_file(struct process* process, int fd);

int process_open_file(struct process* process, int dirfd, const char* path, int mode, int flags);

int process_close_file(struct process* process, int fd);

ssize_t process_read_file(struct process* process, int fd, char* buffer, size_t size);

off_t process_file_seek(struct process* process, int fd, off_t offset, int whence); 

int process_stat(struct process* process, int fd, struct stat* stat);

int process_readdir(struct process* process, int fd, struct dirent* dirent);

int process_get_working_dir(struct process* process, char* buffer, size_t size);

int process_set_working_dir(struct process* process, const char* buffer);

int process_create_thread(struct process* process, void* entry_ptr, void* arg, pid_t* tid, size_t stack_size);

#endif