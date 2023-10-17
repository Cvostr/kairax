#include "proc/thread.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/mem_layout.h"
#include "cpu/gdt.h"
#include "x64_context.h"
#include "kstdlib.h"

struct x64_uthread {
    struct x64_uthread* this;
};

struct thread* create_kthread(struct process* process, void (*function)(void))
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
    //Назначить сегмент из GDT
    uint32_t selector = GDT_BASE_KERNEL_DATA_SEG; //kernel data
    ctx->ds = (selector);
    ctx->es = (selector);
    ctx->ss = (selector);
    //поток в пространстве ядра
    ctx->cs = GDT_BASE_KERNEL_CODE_SEG;
    //Состояние
    thread->state = THREAD_RUNNABLE;

    return thread;
}

struct thread* create_thread(struct process* process, void* entry, void* arg1, size_t stack_size, struct main_thread_create_info* info)
{
    if (!process) {
        return NULL;
    }

    if (stack_size < STACK_SIZE) {
        stack_size = STACK_SIZE;
    }

    // Создать объект потока в памяти
    struct thread* thread = new_thread(process);
    // Данный поток работает в непривилегированном режиме
    thread->is_userspace = 1;
    // Выделить место под стек в памяти процесса
    thread->stack_ptr = process_alloc_stack_memory(process, stack_size);
    thread->kernel_stack_ptr = P2V(pmm_alloc_page());

    if (process->tls) {
        // TLS должно также включать в себя
        size_t required_tls_size = process->tls_size + sizeof(struct x64_uthread);
        // Выделить память и запомнить адрес начала TLS
        thread->tls = (void*)process_brk(process, process->brk + required_tls_size) - required_tls_size;
        // Копировать данные TLS из процесса
        vm_memcpy(process->vmemory_table, (virtual_addr_t)thread->tls, process->tls, process->tls_size);

        struct x64_uthread uthread;
        uthread.this = thread->tls + process->tls_size;

        // Копировать данные структуры
        vm_memcpy(process->vmemory_table, (virtual_addr_t)thread->tls + process->tls_size, &uthread, sizeof(struct x64_uthread));
    }

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
    ctx->ds = (selector);
    ctx->es = (selector);
    ctx->ss = (selector) | 0b11;
    //поток в пространстве ядра
    ctx->cs = GDT_BASE_USER_CODE_SEG | 0b11;    // сегмент кода пользователя
    //Состояние
    thread->state = THREAD_RUNNABLE;

    if (info) {
        uint64_t* stack_new_pos = (uint64_t*) (thread->stack_ptr);

        // Поместить в стек argc
        stack_new_pos -= 1;
        vm_memcpy(process->vmemory_table, stack_new_pos, &info->argc, sizeof(int));

        // Поместить в стек argv
        stack_new_pos -= 1;
        vm_memcpy(process->vmemory_table, stack_new_pos, &info->argv, sizeof(char*));

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

        //ctx->rbp = (uint64_t)stack_aux_pos;
        ctx->rsp = (uint64_t)stack_new_pos;
    }

    return thread;
}