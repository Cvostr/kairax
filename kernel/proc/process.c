#include "process.h"
#include "mem/kheap.h"
#include "mem/pmm.h"
#include "fs/vfs/vfs.h"
#include "string.h"
#include "thread.h"
#include "thread_scheduler.h"
#include "memory/kernel_vmm.h"
#include "errors.h"
#include "kstdlib.h"
#include "proc/elf64/elf64.h"

int last_pid = 0;

struct process*  create_new_process(struct process* parent)
{
    struct process* process = (struct process*)kmalloc(sizeof(struct process));
    memset(process, 0, sizeof(struct process));

    process->name[0] = '\0';
    // Склонировать таблицу виртуальной памяти ядра
    process->vmemory_table = clone_kernel_vm_table();
    atomic_inc(&process->vmemory_table->refs);

    process->brk = 0x0;
    process->threads_stack_top = align_down(USERSPACE_MAX_ADDR, PAGE_SIZE);
    process->pid = last_pid++;
    process->parent = parent;
    // Создать список потоков
    process->threads = create_list();
    process->children = create_list();

    process->mmap_ranges = create_list();

    return process;
}

void free_process(struct process* process)
{
    size_t i = 0;

    for (i = 0; i < list_size(process->threads); i ++) {
        struct thread* thread = list_get(process->threads, i);
        kfree(thread);
    }

    for (i = 0; i < list_size(process->mmap_ranges); i ++) {
        struct mmap_range* range = list_get(process->mmap_ranges, i);
        kfree(range);
    }

    // Освобождение памяти под данные TLS
    if (process->tls) {
        kfree(process->tls);
    }

    // Освободить память списков
    free_list(process->threads);
    free_list(process->children);
    free_list(process->mmap_ranges);

    // Закрыть незакрытые файловые дескрипторы
    for (i = 0; i < MAX_DESCRIPTORS; i ++) {
        if (process->fds[i] != NULL) {
            struct file* fil = process->fds[i];
            file_close(fil);
        }
    }

    // Закрыть объекты рабочей папки
    if (process->workdir) {
        file_close(process->workdir);
    }

    // Переключаемся на основную виртуальную таблицу памяти ядра
    vmm_use_kernel_vm();

    // Уничтожение таблицы виртуальной памяти процесса
    // и освобождение занятых таблиц физической
    free_vm_table(process->vmemory_table);

    // Освободить структуру
    kfree(process);
}

int process_load_arguments(struct process* process, int argc, char** argv, char** args_mem)
{
    int rc = -1;
    // Проверить количество аргументов
    if (argc > PROCESS_MAX_ARGS) {
        // Слишком много аргументов, выйти с ошибкой
        goto exit;
    }
            
    // Вычисляем память, необходимую под массив указателей + строки аргументов
    size_t args_array_size = sizeof(char*) * argc;
    size_t reqd_size = args_array_size;

    // Прибавляем длины строк аргументов
    for (int i = 0; i < argc; i ++) {
        reqd_size += strlen(argv[i]) + 1;
    }

    // Проверка суммарного размера аргументов в памяти
    if (reqd_size > PROCESS_MAX_ARGS_SIZE) {
        // Суммарный размер аргуменов большой, выйти с ошибкой
        rc = -ERROR_ARGS_BUFFER_BIG;
        goto exit;
    }

    // Выделить память процесса под аргументы
    *args_mem = (char*)process_brk(process, process->brk + reqd_size) - reqd_size;

    // Копировать аргументы
    off_t pointers_offset = 0;
    off_t args_offset = args_array_size;

    for (int i = 0; i < argc; i ++) {
        // Длина строки аргумента с терминатором
        size_t arg_len = strlen(argv[i]) + 1;

        // Записать адрес
        uint64_t addr = *args_mem + args_offset;
        vm_memcpy(process->vmemory_table, *args_mem + pointers_offset, &addr, sizeof(uint64_t));

        // Записать строку аргумента
        vm_memcpy(process->vmemory_table, *args_mem + args_offset, argv[i], arg_len);

        // Увеличить смещения
        args_offset += arg_len;
        pointers_offset += sizeof(char*);
    }

    rc = 0;
exit:
    return rc;
}

int process_alloc_memory(struct process* process, uintptr_t start, uintptr_t size, uint64_t flags)
{
    // Выравнивание
    uintptr_t start_aligned = align_down(start, PAGE_SIZE); // выравнивание в меньшую сторону
    uintptr_t size_aligned = align(size, PAGE_SIZE); //выравнивание в большую сторону

    // Добавить диапазон
    struct mmap_range* range = kmalloc(sizeof(struct mmap_range));
    range->base = start_aligned;
    range->length = size_aligned;
    range->protection = flags;
    process_add_mmap_region(process, range);

    // Добавить страницы в таблицу
    for (uintptr_t address = start_aligned; address < start_aligned + size_aligned; address += PAGE_SIZE) {
        int rc = vm_table_map(process->vmemory_table, address, (physical_addr_t)pmm_alloc_page(), flags);

        if (address + PAGE_SIZE > process->brk)
            process->brk = address + PAGE_SIZE;
    }

    return 0;
}

void* process_alloc_stack_memory(struct process* process, size_t stack_size)
{
    // Добавляем защитную страницу после вершины стека
    process->threads_stack_top -= PAGE_SIZE;
    // Выравнивание размера стека
    stack_size = align(stack_size, PAGE_SIZE);
    // Начало памяти под стек
    uint64_t mem_begin = process->threads_stack_top - stack_size;

    // Добавить диапазон памяти к процессу
    struct mmap_range* range = kmalloc(sizeof(struct mmap_range));
    range->base = mem_begin;
    range->length = stack_size;
    range->protection = PAGE_PROTECTION_USER | PAGE_PROTECTION_WRITE_ENABLE;
    process_add_mmap_region(process, range);

    for (uintptr_t address = mem_begin; address < mem_begin + stack_size; address += PAGE_SIZE) {
        vm_table_map(process->vmemory_table, address, pmm_alloc_page(), range->protection);
    }

    void* result = (void*)process->threads_stack_top;
    process->threads_stack_top -= stack_size;

    return result;
}

struct file* process_get_file(struct process* process, int fd)
{
    struct file* result = NULL;
    acquire_spinlock(&process->fd_lock);

    if (fd < 0 || fd >= MAX_DESCRIPTORS)
        goto exit;

    result = process->fds[fd];

exit:
    release_spinlock(&process->fd_lock);
    return result;
}

int process_add_file(struct process* process, struct file* file)
{
    int fd = -ERROR_TOO_MANY_OPEN_FILES;

    acquire_spinlock(&process->fd_lock);

    for (int i = 0; i < MAX_DESCRIPTORS; i ++) {
        if (process->fds[i] == NULL) {
            process->fds[i] = file;
            fd = i;
            goto exit;
        }
    }

exit:
    release_spinlock(&process->fd_lock);
    return fd;
}

int process_close_file(struct process* process, int fd)
{
    int rc = 0;

    if (fd < 0 || fd >= MAX_DESCRIPTORS) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    acquire_spinlock(&process->fd_lock);

    struct file* file = process->fds[fd];

    if (file != NULL) {
        file_close(file);
        process->fds[fd] = NULL;
        goto exit;
    } else {
        rc = -ERROR_BAD_FD;
    }

exit:
    release_spinlock(&process->fd_lock);
    return rc;
}

void process_add_mmap_region(struct process* process, struct mmap_range* region)
{
    acquire_spinlock(&process->mmap_lock);

    list_add(process->mmap_ranges, region);

    release_spinlock(&process->mmap_lock);
}

struct mmap_range* process_get_range_by_addr(struct process* process, uint64_t addr)
{
    for (size_t i = 0; i < list_size(process->mmap_ranges); i ++) {
        struct mmap_range* range = list_get(process->mmap_ranges, i);
        
        uint64_t max_addr = range->base + range->length;
        if (addr >= range->base && addr < max_addr) {
            return range;
        }
    }

    return NULL;
}

int process_handle_page_fault(struct process* process, uint64_t address)
{
    acquire_spinlock(&process->mmap_lock);
    
    struct mmap_range* range = process_get_range_by_addr(process, address);
    if (range == NULL) {
        return 0;
    }

    uint64_t aligned_base = align_down(range->base, PAGE_SIZE);
    size_t pages_count = range->length / PAGE_SIZE;

    for (size_t page_i = 0; page_i < pages_count; page_i ++) {
        vm_table_map(process->vmemory_table, aligned_base + page_i * PAGE_SIZE, pmm_alloc_page(), range->protection);
    }

    release_spinlock(&process->mmap_lock);
    return 1;
}

int process_get_relative_direntry(struct process* process, int dirfd, const char* path, struct dentry** result)
{
    if (dirfd == FD_CWD && process->workdir) {
        // Открыть относительно рабочей директории
        *result = process->workdir->dentry;
        
    } else if (!vfs_is_path_absolute(path)) {
        // Открыть относительно другого файла
        // Получить файл по дескриптору
        struct file* dirfile = process_get_file(process, dirfd);

        if (dirfile) {

            // проверить тип inode от dirfd
            if ( !(dirfile->inode->mode & INODE_TYPE_DIRECTORY)) {
                return -ERROR_NOT_A_DIRECTORY;
            }

            *result = dirfile->dentry;
        } else {
            // Файл не нашелся, а путь не является абсолютным - выходим
            return -ERROR_BAD_FD;
        }
    }

    return 0;
}