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
#include "elf_process_loader.h"
#include "process_list.h"
#include "stdio.h"

struct process*  create_new_process(struct process* parent)
{
    struct process* process = (struct process*)kmalloc(sizeof(struct process));
    if (process == NULL) {
        return NULL;
    }
    memset(process, 0, sizeof(struct process));

    process->type = OBJECT_TYPE_PROCESS;
    process->name[0] = '\0';
    // Склонировать таблицу виртуальной памяти ядра
    process->vmemory_table = clone_kernel_vm_table();
    if (process->vmemory_table == NULL) {
        kfree(process);
        return NULL;
    }

    atomic_inc(&process->vmemory_table->refs);

    process->brk = 0x0;

    // Заполнить обработчики сигналов значениями по умолчанию
    size_t nsignals = sizeof(process->sigactions) / sizeof(struct proc_sigact);
    for (size_t i = 0; i < nsignals; i ++)
    {
        struct proc_sigact* sigact = &process->sigactions[i];
        sigact->handler = SIG_DFL;
        sigact->flags = 0;
        sigact->sigmask = 0;
    }
    
    // Создать список потоков
    process->threads = create_list();
    process->children = create_list();

    process->mmap_ranges = create_list();

    if (parent) {
        process->process_group = parent->process_group;
        acquire_spinlock(&parent->children_lock);
        process->parent = parent;
        list_add(parent->children, process);
        release_spinlock(&parent->children_lock);
    }

    return process;
}

void process_set_name(struct process* process, const char* name)
{
    strncpy(process->name, name, PROCESS_NAME_MAX_LEN - 1);
}

void free_process(struct process* process)
{
    // Данную функцию можно вызывать только для процессов - зомби
    if (process->state != STATE_ZOMBIE)
        return;

    // Удалить информацию о потоках и освободить страницу с стеком ядра
    for (size_t i = 0; i < list_size(process->threads); i ++) {
        struct thread* thread = list_get(process->threads, i);

        while (thread->state != STATE_ZOMBIE);

        process_remove_from_list(thread);
        // Уничтожить объект потока
        thread_destroy(thread);
    }

    // Освободить память под список потоков
    free_list(process->threads);

    // Освободить структуру
    kfree(process);
}

void process_free_resources(struct process* process)
{
    size_t i = 0;

    size_t ranges = list_size(process->mmap_ranges);
    for (i = 0; i < ranges; i ++) 
    {
        struct mmap_range* range = list_get(process->mmap_ranges, i);
        mmap_region_unref(range);
    }

    // Освобождение памяти под данные TLS
    if (process->tls) {
        kfree(process->tls);
    }

    // Освободить память списков
    free_list(process->children);
    free_list(process->mmap_ranges);

    // Закрыть незакрытые файловые дескрипторы
    for (i = 0; i < MAX_DESCRIPTORS; i ++) {
        if (process->fds[i] != NULL) {
            struct file* fil = process->fds[i];
            file_close(fil);
        }
    }

    // Закрыть объект рабочей папки
    if (process->pwd) {
        dentry_close(process->pwd);
    }

    // Переключаемся на основную виртуальную таблицу памяти ядра
    vmm_use_kernel_vm();

    struct vm_table* vmtable = process->vmemory_table;
   
    // Зануление, чтобы планировщик не переключал таблицу
    process->vmemory_table = NULL;

    // Уничтожение таблицы виртуальной памяти процесса
    // и освобождение занятых таблиц физической
    free_vm_table(vmtable);
}

int process_load_arguments(struct process* process, int argc, char** argv, char** args_mem, int add_null)
{
    int i;
    // Вычисляем память, необходимую под массив указателей + строки аргументов
    size_t args_array_size = sizeof(char*) * (argc + add_null);
    size_t reqd_size = args_array_size;

    // Прибавляем длины строк аргументов
    for (i = 0; i < argc; i ++) {
        reqd_size += strlen(argv[i]) + 1;
    }

    // Выделить память процесса под аргументы
    *args_mem = (char*)process_brk(process, process->brk + reqd_size) - reqd_size;

    // Копировать аргументы
    off_t pointers_offset = 0;
    off_t args_offset = args_array_size;

    for (i = 0; i < argc; i ++) {
        // Длина строки аргумента с терминатором
        size_t arg_len = strlen(argv[i]) + 1;

        // Записать адрес
        uint64_t addr = *args_mem + args_offset;
        vm_memcpy(process->vmemory_table, *args_mem + pointers_offset, &addr, sizeof(uint64_t));

        // Записать строку аргумента
        vm_memcpy(process->vmemory_table, (uint64_t) (*args_mem + args_offset), argv[i], arg_len);

        // Увеличить смещения
        args_offset += arg_len;
        pointers_offset += sizeof(char*);
    }

    if (add_null > 0)
    {
        uint64_t zero = 0;
        vm_memcpy(process->vmemory_table, *args_mem + pointers_offset, &zero, sizeof(uint64_t));
    }

    return 0;
}

int process_is_userspace_region(struct process* process, uintptr_t base, size_t len, int protection)
{
    uintptr_t top_address = base + len;

    // Проверим, что адрес не выходит за пределы пользовательского пространства
    if (top_address >= USERSPACE_MAX_ADDR) 
    {
        return FALSE;
    }

    // Проверим по первой странице
    if (top_address < 0x1000)
    {
        return FALSE;
    }

    // Есть ли диапазон для этого адреса
    struct mmap_range* range = process_get_region_by_addr(process, base);
    if (range == NULL)
    {
        return FALSE;
    }

    // Проверим, что адрес и размер входят в область
    if (top_address > (range->base + range->length))
    {
        return FALSE;
    }

    if (protection != 0)
    {
        if ((range->protection & protection) != protection)
        {
            return FALSE;
        }
    }

    return TRUE;
}

int process_is_userspace_string(struct process* process, uintptr_t base)
{
    uintptr_t addr = base;

    // Проверим, что адрес не выходит за пределы пользовательского пространства
    if (base >= USERSPACE_MAX_ADDR) 
    {
        return FALSE;
    }

    // Проверим по первой странице
    if (base < 0x1000)
    {
        return FALSE;
    }

    struct mmap_range* range = NULL;
    while ((range = process_get_region_by_addr(process, addr)) != NULL)
    {
        // Ищем терминирущий 0 в строке
        for (; addr < range->base + range->length; addr ++)
        {
            char* c = addr;
            if (*c == '\0')
            {
                // Нашли - строка в порядке
                return TRUE;
            }
        }
    }

    return FALSE;
}

int process_alloc_memory(struct process* process, uintptr_t start, uintptr_t size, uint64_t flags)
{
    // Выравнивание
    uintptr_t start_aligned = align_down(start, PAGE_SIZE); // выравнивание в меньшую сторону
    uintptr_t end_addr = align(start + size, PAGE_SIZE);

    // Добавить диапазон
    struct mmap_range* range = new_mmap_region();
    range->base = start_aligned;
    range->length = end_addr - start_aligned;
    range->protection = flags;
    process_add_mmap_region(process, range);

    // Добавить страницы в таблицу
    for (uintptr_t address = start_aligned; address < end_addr; address += PAGE_SIZE) {
        // Выделение страницы физической памяти
        void* page_addr = pmm_alloc_page();
        // Предварительное зануление
        memset(P2V(page_addr), 0, PAGE_SIZE);
        // Добавление страницы в адресное пространство процесса
        int rc = vm_table_map(process->vmemory_table, address, (uint64_t) page_addr, flags);

        if (address + PAGE_SIZE > process->brk)
            process->brk = address + PAGE_SIZE;
    }

    return 0;
}

struct mmap_range* process_alloc_stack_memory(struct process* process, size_t stack_size, int need_map)
{
    // Создаем внизу защитную область в размере страницы
    // Чтобы при попытке взаимодействия завершить программу с SIGSEGV
    // Выравнивание размера стека с прибавлением размера "защиты"
    stack_size = align(stack_size, PAGE_SIZE) + PAGE_SIZE;
    
    // Начало памяти под стек
    uint64_t mem_begin = process_get_free_addr(process, stack_size, align_down(USERSPACE_MMAP_ADDR, PAGE_SIZE));

    // Добавить диапазон памяти к процессу
    struct mmap_range* range = new_mmap_region();
    range->base = mem_begin;
    range->length = stack_size;
    range->protection = PAGE_PROTECTION_USER | PAGE_PROTECTION_WRITE_ENABLE;
    range->flags = MAP_STACK;
    process_add_mmap_region(process, range);

    // Замапить верхнюю страницу, если необходимо
    // Пропускаем первую защитную страницу
    // Это надо чтобы сложить AUX вектор в стек при запуске программы
    if (need_map == TRUE) 
    {
        uintptr_t stack_top = mem_begin + stack_size; 
        uintptr_t top_addr = stack_top - PAGE_SIZE;

        for (uintptr_t address = top_addr; address < stack_top; address += PAGE_SIZE) 
        {
            vm_table_map(process->vmemory_table, address, pmm_alloc_page(), range->protection);
        }
    }

    return range;
}

uintptr_t process_brk(struct process* process, uint64_t addr)
{
    uint64_t protection = PAGE_PROTECTION_USER | PAGE_PROTECTION_WRITE_ENABLE;
    //Выравнивание до размера страницы в большую сторону
    uintptr_t uaddr = align(addr, PAGE_SIZE);

    // Добавить диапазон памяти к процессу
    struct mmap_range* range = new_mmap_region();
    range->base = process->brk;
    range->length = uaddr - process->brk;
    range->protection = protection;
    process_add_mmap_region(process, range);

    //До тех пор, пока адрес конца памяти процесса меньше, выделяем страницы
    while ((uint64_t)uaddr > process->brk) {
        vm_table_map(process->vmemory_table, process->brk, (physical_addr_t)pmm_alloc_page(), protection);
        process->brk += PAGE_SIZE;
    }

    return process->brk;
}

struct process* process_get_child_by_id(struct process* process, pid_t id)
{
    struct process* result = NULL;
    acquire_spinlock(&process->children_lock);
    
    for (size_t i = 0; i < list_size(process->children); i ++) {
        struct process* child = list_get(process->children, i);
        if (child->pid == id) {
            result = child;
            goto exit;
        }
    }

exit:
    release_spinlock(&process->children_lock);
    return result;
}

struct thread* process_get_thread_by_id(struct process* process, pid_t id)
{
    struct thread* result = NULL;
    acquire_spinlock(&process->threads_lock);
    
    for (size_t i = 0; i < list_size(process->threads); i ++) {
        struct thread* child = list_get(process->threads, i);
        if (child->id == id) {
            result = child;
            goto exit;
        }
    }

exit:
    release_spinlock(&process->threads_lock);
    return result;
}

struct process* process_get_first_zombie(struct process* process)
{
    struct process* result = NULL;
    acquire_spinlock(&process->children_lock);
    
    for (size_t i = 0; i < list_size(process->children); i ++) {
        struct process* child = list_get(process->children, i);
        if (child->state == STATE_ZOMBIE) {
            result = child;
            goto exit;
        }
    }

exit:
    release_spinlock(&process->children_lock);
    return result;
}

void  process_remove_child(struct process* process, struct process* child)
{
    acquire_spinlock(&process->children_lock);
    list_remove(process->children, child);
    release_spinlock(&process->children_lock);
}

void  process_remove_thread(struct process* process, struct thread* thread)
{
    acquire_spinlock(&process->threads_lock);
    list_remove(process->threads, thread);
    release_spinlock(&process->threads_lock);
}

struct file* process_get_file(struct process* process, int fd)
{
    return process_get_file_ex(process, fd, FALSE);
}

struct file* process_get_file_ex(struct process* process, int fd, int acquire)
{
    struct file* result = NULL;
    acquire_spinlock(&process->fd_lock);

    if (fd < 0 || fd >= MAX_DESCRIPTORS)
        goto exit;

    result = process->fds[fd];

    // Если файл найден и флаг стоит - увеличим счетчик ссылок
    if (acquire == TRUE && result != NULL)
    {
        file_acquire(result);
    }

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
            file_acquire(file);
            fd = i;
            goto exit;
        }
    }

exit:
    release_spinlock(&process->fd_lock);
    return fd;
}

int process_add_file_at(struct process* process, struct file* file, int fd)
{
    if (fd < 0 || fd >= MAX_DESCRIPTORS) {
        return -ERROR_BAD_FD;
    }

    acquire_spinlock(&process->fd_lock);

    if (process->fds[fd] != NULL) {
        // На этой позиции уже есть файл - закрываем
        file_close(process->fds[fd]);
    }

    // Добавить файл и увеличить счетчик ссылок
    process->fds[fd] = file;
    file_acquire(file);

    release_spinlock(&process->fd_lock);

    return 0;
}

int process_close_file(struct process* process, int fd)
{
    int rc = 0;

    if (fd < 0 || fd >= MAX_DESCRIPTORS) {
        return -ERROR_BAD_FD;
    }

    acquire_spinlock(&process->fd_lock);

    struct file* file = process->fds[fd];

    if (file != NULL) 
    {
        // Закрыть объект открытого файла
        file_close(file);
        // Снимаем CLOEXEC
        process_set_cloexec(process, fd, 0);
        // Освобождаем номер дескриптора
        process->fds[fd] = NULL;
        // Выход
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
    mmap_region_ref(region);

    release_spinlock(&process->mmap_lock);
}

struct mmap_range* process_get_region_by_addr(struct process* process, uint64_t addr)
{
    struct list_node* current_area_node = process->mmap_ranges->head;

    while (current_area_node != NULL)
    {
        struct mmap_range* range = current_area_node->element;
        
        uint64_t max_addr = range->base + range->length - 1;
        if (addr >= range->base && addr <= max_addr) {
            return range;
        }

        current_area_node = current_area_node->next;
    }
    
    return NULL;
}

int is_regions_collide(struct mmap_range* region1, struct mmap_range* region2) {

    uint64_t region1_end = region1->base + region1->length - 1; // 0 - 4095
    uint64_t region2_end = region2->base + region2->length - 1; // 0 - 4095

    int a = region1->base >= region2->base && region1->base <= region2_end; 
    int b = region1_end >= region2->base && region1_end <= region2_end; 

    int c = region2->base >= region1->base && region2->base <= region1_end; 
    int d = region2_end >= region1->base && region2_end <= region1_end; 

    return a || b || c || d;
}

void* process_get_free_addr(struct process* process, size_t length, uintptr_t hint)
{
    void* result = NULL;
    int collides = FALSE;
    size_t i;
    acquire_spinlock(&process->mmap_lock);
    size_t mappings_count = list_size(process->mmap_ranges);

    // Подготовим временную структуру для проверок
    struct mmap_range new_range;
    new_range.length = length;

    if (hint > 0) {
        // Есть указание разместить регион по адресу
        new_range.base = hint;

        for (i = 0; i < mappings_count; i ++) {

            struct mmap_range* region = list_get(process->mmap_ranges, i);

            if (is_regions_collide(&new_range, region)) {
                // Регионы пересекаются, выходим
                collides = TRUE;
                break;
            }
        }

        if (collides == FALSE) {
            result = (void*) hint;
            goto exit;
        }
    }

    // Ищем адрес после какого-либо другого региона
    for (i = 0; i < mappings_count; i ++) {

        struct mmap_range* range = list_get(process->mmap_ranges, i);

        // Адрес начала предполагаемого диапазона
        new_range.base = range->base + range->length;

        // Начало нового региона должно быть после hint и brk
        // Также конец не должен вылазить за границы адресного пространства процесса
        if (new_range.base < hint || 
            new_range.base < process->brk || 
            new_range.base + length >= USERSPACE_MAX_ADDR) 
        {
            continue;
        }

        collides = 0;

        for (size_t j = 0; j < mappings_count; j ++) {

            if (i == j)
                continue;

            struct mmap_range* region = list_get(process->mmap_ranges, j);
        
            if (is_regions_collide(&new_range, region)) {
                // Регионы пересекаются, выходим
                collides = 1;
                break;
            }
        }

        if (collides == 0) {
            result = (void*) new_range.base;
            goto exit;
        }
    }

exit:
    release_spinlock(&process->mmap_lock);
    return result;
}

int process_handle_page_fault(struct process* process, uint64_t address)
{
    if (try_acquire_spinlock(&process->mmap_lock)) {
    
        struct mmap_range* range = process_get_region_by_addr(process, address);
        if (range == NULL) {
            return 0;
        }

        // Выровнять таблицу вниз
        uint64_t aligned_address = align_down(address, PAGE_SIZE);

        if ((range->flags & MAP_STACK) == MAP_STACK && aligned_address == range->base) {
            // Защитная страница стека, чтобы вызвать SIGSEGV при переполнении 
            return 0;
        }

        // Выделить страницу
        char* paddr = pmm_alloc_page();
        // Предварительное зануление
        memset(P2V(paddr), 0, PAGE_SIZE);
        // замаппить
        int rc = vm_table_map(process->vmemory_table, aligned_address, paddr, range->protection);

        release_spinlock(&process->mmap_lock);
        return 1;
    }
    printk("Locked in process_handle_page_fault(%s)\n", ulltoa(address, 16));
    // Дождемся разблокировки  
    return 1;
}

void process_set_cloexec(struct process* process, int fd, int value)
{
    int lindex = fd / CLOEXEC_INT_SIZE;
    int fdindex = fd % CLOEXEC_INT_SIZE;

    if (value == 0) {
        // Снять старый бит
        process->close_on_exec[lindex] &= ~(1ULL << fdindex); 
    } else {
        process->close_on_exec[lindex] |= (1ULL << fdindex);
    }
}

int process_get_cloexec(struct process* process, int fd)
{
    int lindex = fd / CLOEXEC_INT_SIZE;
    int fdindex = fd % CLOEXEC_INT_SIZE;
    return (process->close_on_exec[lindex] >> fdindex) & 1;
}

int process_get_relative_direntry(struct process* process, int dirfd, const char* path, struct dentry** result)
{
    if (dirfd == FD_CWD && process->pwd) {
        // Открыть относительно рабочей директории
        *result = process->pwd;
        
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

int process_get_relative_direntry1(struct process* process, int dirfd, const char* path, struct dentry** result)
{
    if (dirfd == FD_CWD && process->pwd) {
        // Открыть относительно рабочей директории
        *result = process->pwd;
        dentry_open(*result);
        
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
            dentry_open(*result);
            
        } else {
            // Файл не нашелся, а путь не является абсолютным - выходим
            return -ERROR_BAD_FD;
        }
    }

    return 0;
}

int process_open_file_relative(struct process* process, int dirfd, const char* path, int flags, struct file** file)
{
    int fopen_flag = 0;
    if ((flags & AT_SYMLINK_NOFOLLOW) == AT_SYMLINK_NOFOLLOW)
    {
        fopen_flag |= (O_NOFOLLOW | O_PATH);
    }

    if (flags & AT_EMPTY_PATH) {
        // Дескриптор файла передан в dirfd
        // Получим файл с увеличением счетчика ссылок
        *file = process_get_file_ex(process, dirfd, TRUE);
    } else if (dirfd == FD_CWD && process->pwd) {
        // Указан путь относительно рабочей директории
        *file = file_open(process->pwd, path, fopen_flag, 0);
    } else {
        // Открываем файл относительно dirfd
        struct file* dirfile = process_get_file(process, dirfd);

        if (dirfile) {

            // проверить тип inode от dirfd
            if ( !(dirfile->inode->mode & INODE_TYPE_DIRECTORY)) {
                return -ERROR_NOT_A_DIRECTORY;
            }

        } else {
            // Не найден дескриптор dirfd
            return -ERROR_BAD_FD;
        }

        // Открыть файл
        *file = file_open(dirfile->dentry, path, fopen_flag, 0);
    }

    if (*file == NULL) {
        // Не получилось открыть файл, выходим
        if (flags & AT_EMPTY_PATH) {
            return -ERROR_BAD_FD;
        } else {
            return -ERROR_NO_FILE;
        }
    }

    return 0;
}