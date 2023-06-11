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
    thread->thread_id   = last_id++;
    thread->process     = (process);
    return thread;
}

thread_t* create_kthread(process_t* process, void (*function)(void))
{
    // Создать объект потока в памяти
    thread_t* thread    = new_thread(process);
    // Выделить место под стек в памяти процесса
    thread->stack_ptr   = process_brk(process, process->brk + STACK_SIZE);
    // Добавить поток в список потоков процесса
    list_add(process->threads, thread);
    //Подготовка контекста
    thread_frame_t* ctx = &thread->context;
    //Установить адрес функции
    ctx->rip = (uint64_t)P2V((uint64_t)function);
    ctx->rflags = 0x286;
    //Установить стек для потока
    ctx->rbp = (uint64_t)thread->stack_ptr;
    ctx->rsp = (uint64_t)thread->stack_ptr;
    //Назначить сегмент из GDT
    uint32_t selector = GDT_BASE_KERNEL_DATA_SEG; //kernel data
    thread->context.ds = (selector);
    thread->context.es = (selector);
    thread->context.fs = (selector);
    thread->context.gs = (selector);
    thread->context.ss = (selector);
    //поток в пространстве ядра
    thread->context.cs = GDT_BASE_KERNEL_CODE_SEG;
    //Состояние
    thread->state = THREAD_CREATED;

    return thread;
}

thread_t* create_thread(process_t* process, uintptr_t entry)
{
    // Создать объект потока в памяти
    thread_t* thread    = new_thread(process);
    // Данный поток работает в непривилегированном режиме
    thread->is_userspace = 1;
    // Выделить место под стек в памяти процесса
    thread->stack_ptr = process_brk(process, process->brk + STACK_SIZE);
    // Добавить поток в список потоков процесса
    list_add(process->threads, thread);
    //Подготовка контекста
    thread_frame_t* ctx = &thread->context;
    //Установить адрес функции
    ctx->rip = entry;
    ctx->rflags = 0x286;
    //Установить стек для потока
    ctx->rbp = (uint64_t)thread->stack_ptr;
    ctx->rsp = (uint64_t)thread->stack_ptr;
    //Назначить сегмент из GDT
    uint32_t selector = GDT_BASE_USER_DATA_SEG; // сегмент данных пользователя
    thread->context.ds = (selector);
    thread->context.es = (selector);
    thread->context.fs = (selector);
    thread->context.gs = (selector);
    thread->context.ss = (selector);
    //поток в пространстве ядра
    thread->context.cs = GDT_BASE_USER_CODE_SEG;    // сегмент кода пользователя
    //Состояние
    thread->state = THREAD_CREATED;

    return thread;
}