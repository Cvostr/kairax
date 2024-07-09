#ifndef _PROCESS_H
#define _PROCESS_H

#include "kairax/list/list.h"
#include "fs/vfs/file.h"
#include "fs/vfs/stat.h"
#include "mem/paging.h"
#include "process_list.h"
#include "kairax/signal.h"
#include "mem/vm_area.h"

#define MAX_DESCRIPTORS         64
#define PROCESS_MAX_ARGS        65535
#define PROCESS_MAX_ARGS_SIZE   (128ULL * 1024 * 1024)

#define STATE_RUNNING                  0    // Работает
#define STATE_RUNNABLE                 1
#define STATE_INTERRUPTIBLE_SLEEP      2    // В ожидании ресурса
#define STATE_UNINTERRUPTIBLE_SLEEP    3    // В ожидании ресурса внутри системного вызова
#define STATE_ZOMBIE                   4    // Завершен, но не обработан родителем

#define OBJECT_TYPE_PROCESS   1
#define OBJECT_TYPE_THREAD    2

typedef unsigned long long CLOEXEC_INT_TYPE;
#define CLOEXEC_INT_SIZE    sizeof(CLOEXEC_INT_TYPE)

struct process {
    // Тип объекта
    int                 type;
    // ID процесса
    pid_t               pid;
    // Состояние
    int                 state;
    // Код выхода
    int                 code;
    // Название
    char                name[30];
    // Процесс - родитель
    struct process*     parent;
    //
    uid_t               uid;
    uid_t               euid;
    // Сигналы
    sigset_t            pending_signals;
    sigset_t            blocked_signals;
    spinlock_t          sighandler_lock;
    // Адрес, после которого загружен код программы и линковщика
    uint64_t            brk;
    // Рабочая папка
    spinlock_t          pwd_lock;
    struct dentry*      pwd;
    // Таблица виртуальной памяти процесса
    struct vm_table*    vmemory_table;  
    // Связный список потоков
    list_t*             threads;  
    spinlock_t          threads_lock;
    // Связный список потомков
    list_t*             children;
    spinlock_t          children_lock;
    // Указатели на открытые файловые дескрипторы
    struct file*        fds[MAX_DESCRIPTORS];
    CLOEXEC_INT_TYPE    close_on_exec[MAX_DESCRIPTORS / CLOEXEC_INT_SIZE];
    spinlock_t          fd_lock;
    // начальные данные для TLS
    char*               tls;
    size_t              tls_size;
    // Карта адресного пространства
    list_t*             mmap_ranges;
    spinlock_t          mmap_lock;
    // Для ожиданий
    spinlock_t          wait_lock;
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

struct process* process_get_child_by_id(struct process* process, pid_t id);

struct thread* process_get_thread_by_id(struct process* process, pid_t id);

void  process_remove_child(struct process* process, struct process* child);

void  process_remove_thread(struct process* process, struct thread* thread);

void process_become_zombie(struct process* process);

void free_process(struct process* process);

// Установить адрес конца памяти процесса
uintptr_t        process_brk(struct process* process, uint64_t addr);

int process_alloc_memory(struct process* process, uintptr_t start, uintptr_t size, uint64_t flags);

void* process_alloc_stack_memory(struct process* process, size_t stack_size, int need_map);

int process_is_userspace_region(struct process* process, uintptr_t base, size_t len);

// Получает объект открытого файла по номеру дескриптора
struct file* process_get_file(struct process* process, int fd);

// Загружает аргументы командной строки в адресное пространство процесса
// args_mem - указатель на данные аргументов в адресном пространстве процесса
int process_load_arguments(struct process* process, int argc, char** argv, char** args_mem, int add_null);

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

// Получить адрес начала свободного региона памяти адресного пространства процесса
void* process_get_free_addr(struct process* process, size_t length, uintptr_t hint);

// Обработать исключение Page Fault
int process_handle_page_fault(struct process* process, uint64_t address);

int process_create_thread(struct process* process, void* entry_ptr, void* arg, pid_t* tid, size_t stack_size);

int process_send_signal(struct process* process, int signal);

void process_set_cloexec(struct process* process, int fd, int value);
int process_get_cloexec(struct process* process, int fd);

#endif