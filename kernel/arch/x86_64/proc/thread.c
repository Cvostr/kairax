#include "thread.h"
#include "memory/pmm.h"
#include "string.h"

int last_id = 0;

thread_t* create_new_thread(process_t* process, void (*function)(void)){
    thread_t* thread    = (thread_t*)alloc_page();
    thread->thread_id   = last_id++;
    thread->process     = process;
    thread->stack_ptr   = (thread_t*)alloc_page() + PAGE_SIZE; //FIX LATER
    //Подготовка контекста
    memset((void *)(&(thread->context)), 0x0, sizeof(thread_frame_t));

    thread_frame_t* ctx = &thread->context;
    //Установить адрес функции
    ctx->rip = (uint64_t)function;
    ctx->rflags = 0x286;
    //Установить стек для потока
    ctx->rbp = (uint64_t)thread->stack_ptr;
    ctx->rsp = (uint64_t)thread->stack_ptr;
    //Назначить сегмент из GDT
    uint32_t selector = 0x10; //kernel data
    thread->context.ds = (selector);
    thread->context.es = (selector);
    thread->context.fs = (selector);
    thread->context.gs = (selector);
    //поток в пространстве ядра
    thread->context.cs = 0x8;
    //Состояние
    thread->state = THREAD_CREATED;

    return thread;
}