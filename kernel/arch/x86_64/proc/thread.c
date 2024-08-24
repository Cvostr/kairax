#include "proc/thread.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/mem_layout.h"
#include "cpu/gdt.h"
#include "x64_context.h"
#include "kstdlib.h"
#include "proc/elf_process_loader.h"

struct x64_uthread {
    struct x64_uthread* this;
};

struct thread* create_kthread(struct process* process, void (*function)(void), void* arg)
{
    if (!process || !function) {
        return NULL;
    }
    // Создать объект потока в памяти
    struct thread* thread = new_thread(process);
    // Выделить место под стек
    thread->stack_ptr = P2V(pmm_alloc_page() + PAGE_SIZE);
    memset(thread->stack_ptr, 0, PAGE_SIZE);
    // Добавить поток в список потоков процесса
    list_add(process->threads, thread);
    //Подготовка контекста
    thread_frame_t* ctx = ((thread_frame_t*)thread->stack_ptr) - 1;
    thread->context = ctx;
    //Установить адрес функции
    ctx->rip = (uint64_t)P2V((uint64_t)function);
    ctx->rflags = 0x286;
    //Установить стек для потока
    ctx->rbp = (uint64_t)thread->stack_ptr;
    ctx->rsp = (uint64_t)thread->stack_ptr;
    // Установка аргумента
    ctx->rdi = (uint64_t)arg;
    //Назначить сегмент из GDT
    uint32_t selector = GDT_BASE_KERNEL_DATA_SEG; //kernel data
    ctx->ss = (selector);
    //поток в пространстве ядра
    ctx->cs = GDT_BASE_KERNEL_CODE_SEG;
    //Состояние
    thread->state = STATE_RUNNABLE;
    thread->timeslice = 2;

    return thread;
}

struct thread* create_thread(struct process* process, void* entry, void* arg1, int stack_size, struct main_thread_create_info* info)
{
    if (!process) {
        return NULL;
    }

    int create_stack = stack_size >= 0;

    if (stack_size < STACK_SIZE) {
        stack_size = STACK_SIZE;
    }

    // Создать объект потока в памяти
    struct thread* thread = new_thread(process);

    // Название потока
    strcpy(thread->name, process->name);
    strcat(thread->name, "#");

    // Данный поток работает в непривилегированном режиме
    thread->is_userspace = 1;

    if (create_stack) {
        // Выделить место под стек в памяти процесса
        int map_stack = (info != NULL) ? TRUE : FALSE;
        thread->stack_ptr = process_alloc_stack_memory(process, stack_size, map_stack);
    }
    
    // Выделить память под стек ядра
    thread->kernel_stack_ptr = P2V(pmm_alloc_page());
    memset(thread->kernel_stack_ptr, 0, PAGE_SIZE);

    thread_create_tls(thread);

    // Выделить память под контекст FPU и инициализировать его
    thread->fpu_context = P2V(pmm_alloc_page());
    memset(thread->fpu_context, 0, PAGE_SIZE);
    fpu_context_init(thread->fpu_context);

    // Добавить поток в список потоков процесса
    list_add(process->threads, thread);
    //Подготовка контекста
    void* kernel_stack_top = thread->kernel_stack_ptr + PAGE_SIZE;
    thread_frame_t* ctx = ((thread_frame_t*)kernel_stack_top) - 1;
    thread->context = ctx;
    //Установить адрес функции
    ctx->rip = (uint64_t)entry;
    ctx->rflags = 0x286;
    //Установить стек для потока
    ctx->rbp = (uint64_t)thread->stack_ptr;
    ctx->rsp = (uint64_t)thread->stack_ptr;
    // Установка аргумента
    ctx->rdi = (uint64_t)arg1;
    //Назначить сегмент из GDT
    uint32_t selector = GDT_BASE_USER_DATA_SEG; // сегмент данных пользователя
    ctx->ss = (selector) | 0b11;
    //поток в пространстве ядра
    ctx->cs = GDT_BASE_USER_CODE_SEG | 0b11;    // сегмент кода пользователя
    //Состояние
    thread->state = STATE_RUNNABLE;

    if (info) {
        thread_add_main_thread_info(thread, info);
    }

    return thread;
}

void thread_recreate_on_execve(struct thread* thread, struct main_thread_create_info* info)
{
    struct process* process = thread->process;

    // Название потока
    strcpy(thread->name, process->name);
    strcat(thread->name, "#");

    thread->stack_ptr = process_alloc_stack_memory(process, STACK_SIZE, 1);

    thread_create_tls(thread);
    // Надо сразу применить новый указатель TLS
    cpu_set_fs_base(thread->tls + process->tls_size);

    thread_add_main_thread_info(thread, info);
}

void thread_create_tls(struct thread* thread)
{
    struct process* process = thread->process;
    if (process->tls) {
        // TLS должно также включать в себя struct x64_uthread
        size_t required_tls_size = process->tls_size + sizeof(struct x64_uthread);
        size_t aligned_mem_size = align(required_tls_size, PAGE_SIZE);

        // Выделить память под TLS
        uint64_t mem_begin = process_get_free_addr(process, aligned_mem_size, align_down(USERSPACE_MMAP_ADDR, PAGE_SIZE));
        // Добавить диапазон памяти к процессу
        struct mmap_range* range = kmalloc(sizeof(struct mmap_range));
        range->base = mem_begin;
        range->length = aligned_mem_size;
        range->protection = PAGE_PROTECTION_USER | PAGE_PROTECTION_WRITE_ENABLE;
        range->flags = 0;
        process_add_mmap_region(process, range);
        for (uintptr_t address = mem_begin; address < mem_begin + aligned_mem_size; address += PAGE_SIZE) {
            vm_table_map(process->vmemory_table, address, pmm_alloc_page(), range->protection);
        }

        // Вычислить адрес начала TLS
        thread->tls = mem_begin + aligned_mem_size - required_tls_size; 
        // Копировать данные TLS из процесса
        vm_memcpy(process->vmemory_table, (virtual_addr_t)thread->tls, process->tls, process->tls_size);

        struct x64_uthread uthread;
        uthread.this = thread->tls + process->tls_size;

        // Копировать данные структуры
        vm_memcpy(process->vmemory_table, (virtual_addr_t)thread->tls + process->tls_size, &uthread, sizeof(struct x64_uthread));
    }
}

void thread_add_main_thread_info(struct thread* thread, struct main_thread_create_info* info)
{
    uint64_t* stack_new_pos = (uint64_t*) (thread->stack_ptr);
    struct process* process = thread->process;

    // Поместить в стек argc
    stack_new_pos -= 1;
    vm_memcpy(process->vmemory_table, stack_new_pos, &info->argc, sizeof(int));

    // Поместить в стек argv
    stack_new_pos -= 1;
    vm_memcpy(process->vmemory_table, stack_new_pos, &info->argv, sizeof(char*));

    // Поместить в стек envp
    stack_new_pos -= 1;
    vm_memcpy(process->vmemory_table, stack_new_pos, &info->envp, sizeof(char*));

    // Поместить в стек aux вектор
    for (size_t i = 0; i < info->aux_size; i ++) {
        struct aux_pair* aux_cur = &info->auxv[i];
        // ключ
        stack_new_pos -= 1;
        vm_memcpy(process->vmemory_table, stack_new_pos, &aux_cur->pval, sizeof(uint64_t));
        // значение
        stack_new_pos -= 1;
        vm_memcpy(process->vmemory_table, stack_new_pos, &aux_cur->type, sizeof(uint64_t));
    }

    thread_frame_t* ctx = (thread_frame_t*)thread->context;
    ctx->rsp = (uint64_t)stack_new_pos;
    thread->stack_ptr = ctx->rsp;
}
