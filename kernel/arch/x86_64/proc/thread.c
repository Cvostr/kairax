#include "thread.h"
#include "memory/pmm.h"
#include "string.h"

int last_id = 0;

thread_t* create_new_thread(process_t* process, void (*function)(void)){
    thread_t* thread    = (thread_t*)alloc_page();
    thread->thread_id   = last_id++;
    thread->process     = process;
    thread->context     = ((char*)thread) + sizeof(cpu_context_t);
    thread->stack_ptr   = (thread_t*)alloc_page() + PAGE_SIZE; //FIX LATER
    //Подготовка контекста
    memset((void *)(&(thread->context)), 0x0, sizeof(cpu_context_t));
    thread->context->rip = (uint64_t)function;
    thread->context->rflags = 0x286;
    //назначить адрес стека
    thread->context->rbp = thread->stack_ptr;
    thread->context->rsp = thread->stack_ptr;
    //Назначить сегмент из GDT
    uint32_t selector = 0x10; //kernel data
    thread->context->ds = (selector);
    thread->context->es = (selector);
    thread->context->fs = (selector);
    thread->context->gs = (selector);
    //Состояние
    thread->state = THREAD_CREATED;

    return thread;
}