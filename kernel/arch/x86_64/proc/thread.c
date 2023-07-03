#include "thread.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"
#include "memory/mem_layout.h"
#include "cpu/gdt.h"

int last_id = 0;

thread_t* new_thread(process_t* process)
{
    thread_t* thread    = (thread_t*)kmalloc(sizeof(thread_t));
    memset(thread, 0, sizeof(thread_t));
    thread->id   = last_id++;
    thread->process     = (process);
    return thread;
}

thread_t* create_kthread(process_t* process, void (*function)(void))
{
    if (!process || !function) {
        return NULL;
    }
    // Создать объект потока в памяти
    thread_t* thread = new_thread(process);
    // Выделить место под стек в памяти процесса
    thread->stack_ptr = (void*)process_brk(process, (void*)(process->brk + STACK_SIZE));
    //Переводим адрес стэка в глобальный адрес, доступный из всех таблиц
    physical_addr_t stack_phys_addr = get_physical_address(process->vmemory_table, (uintptr_t)thread->stack_ptr - PAGE_SIZE);
    thread->stack_ptr = P2V(stack_phys_addr + PAGE_SIZE);
    // Добавить поток в список потоков процесса
    list_add(process->threads, thread);
    //Подготовка контекста
    //thread_frame_t* ctx = &thread->context;
    thread_frame_t* ctx = ((thread_frame_t*)thread->stack_ptr) - 1;//&thread->context;
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
    ctx->fs = (selector);
    ctx->ss = (selector);
    //поток в пространстве ядра
    ctx->cs = GDT_BASE_KERNEL_CODE_SEG;
    //Состояние
    thread->state = THREAD_CREATED;

    return thread;
}

thread_t* create_thread(process_t* process, void* entry, void* arg, size_t stack_size)
{
    if (!process) {
        return NULL;
    }

    if (stack_size < 128) {
        stack_size = STACK_SIZE;
    }

    // Создать объект потока в памяти
    thread_t* thread = new_thread(process);
    // Данный поток работает в непривилегированном режиме
    thread->is_userspace = 1;
    // Выделить место под стек в памяти процесса
    thread->stack_ptr = (void*)process_brk(process, process->brk + stack_size);
    thread->kernel_stack_ptr = (void*)process_brk(process, process->brk + STACK_SIZE);

    //if (process->tls) {
    //    thread->tls = (void*)process_brk(process, process->brk + process->tls_size) - process->tls_size;
    //    copy_to_vm(process->vmemory_table, thread->tls, process->tls, process->tls_size);
    //}

    //Переводим адрес стэка в глобальный адрес, доступный из всех таблиц
    physical_addr_t kernel_stack_phys = 
        get_physical_address(process->vmemory_table, (uintptr_t)thread->kernel_stack_ptr - PAGE_SIZE);
    thread->kernel_stack_ptr = P2V(kernel_stack_phys + PAGE_SIZE);
    // Добавить поток в список потоков процесса
    list_add(process->threads, thread);
    //Подготовка контекста
    thread_frame_t* ctx = ((thread_frame_t*)thread->kernel_stack_ptr) - 1;//&thread->context;
    thread->context = ctx;
    //thread_frame_t* ctx = &thread->context;
    //Установить адрес функции
    ctx->rip = (uint64_t)entry;
    ctx->rflags = 0x286;
    //Установить стек для потока
    ctx->rbp = (uint64_t)thread->stack_ptr;
    ctx->rsp = (uint64_t)thread->stack_ptr;
    // Установка аргумента
    ctx->rdi = (uint64_t)arg;
    //Назначить сегмент из GDT
    uint32_t selector = GDT_BASE_USER_DATA_SEG; // сегмент данных пользователя
    ctx->ds = (selector);
    ctx->es = (selector);
    ctx->fs = (selector);
    ctx->ss = (selector) | 0b11;
    //поток в пространстве ядра
    ctx->cs = GDT_BASE_USER_CODE_SEG | 0b11;    // сегмент кода пользователя
    //Состояние
    thread->state = THREAD_CREATED;

    return thread;
}