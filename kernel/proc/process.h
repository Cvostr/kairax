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

enum process_state {
    PROCESS_RUNNING          = 0,    // Работает/может работать
    PROCESS_ZOMBIE           = 4     // Завершен, но не обработан родителем
};

struct process {
    char                name[30];
    // ID процесса
    pid_t               pid;
    enum process_state  state;
    // Адрес, после которого загружен код программы и линковщика
    uint64_t            brk;
    uint64_t            threads_stack_top;

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

    int stdin;
    int stdout;
    int stderr;
};

//Создать новый пустой процесс
struct process*  create_new_process(struct process* parent);

int process_get_relative_direntry(struct process* process, int dirfd, const char* path, struct dentry** result);

int process_open_file_relative(struct process* process, int dirfd, const char* path, int flags, struct file** file, int* close_at_end);

void free_process(struct process* process);

// Установить адрес конца памяти процесса
uintptr_t        process_brk(struct process* process, uint64_t addr);

int process_alloc_memory(struct process* process, uintptr_t start, uintptr_t size, uint64_t flags);

void* process_alloc_stack_memory(struct process* process, size_t stack_size);

// Получает объект открытого файла по номеру дескриптора
struct file* process_get_file(struct process* process, int fd);

// Загружает аргументы командной строки в адресное пространство процесса
// args_mem - указатель на данные аргументов в адресном пространстве процесса
int process_load_arguments(struct process* process, int argc, char** argv, char** args_mem);

// Добавляет файл к процессу с первым свободным номером дескриптора
// Повышает счетчик ссылок
// Возвращает номер дескриптора добавленного файла в этом процессе
int process_add_file(struct process* process, struct file* file);

// Добавляет файл к процессу с указанным номером дескриптора
// Если номер уже занят - производит замену с закрытием предыдущего файла
// Повышает счетчик ссылок
int process_add_file_at(struct process* process, struct file* file, int fd);

// Закрывает файл с указанным номером fd, удаляя его из процесса
// Понижает счетчик ссылок файла
int process_close_file(struct process* process, int fd);

void process_add_mmap_region(struct process* process, struct mmap_range* region);

// Получить объект региона памяти по адресу
struct mmap_range* process_get_region_by_addr(struct process* process, uint64_t addr);

void* process_get_free_addr(struct process* process, size_t length, uintptr_t hint);

// Обработать исключение Page Fault
int process_handle_page_fault(struct process* process, uint64_t address);

int process_create_thread(struct process* process, void* entry_ptr, void* arg, pid_t* tid, size_t stack_size);

#endif