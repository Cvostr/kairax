#include "thread.h"
#include "memory/pmm.h"

int last_id = 0;

thread_t* create_new_thread(void (*function)(void)){
    thread_t* thread    = (thread_t*)alloc_page();
    thread->thread_id   = last_id++;
    //thread->stack_ptr   = ((char*)thread) + PAGE_SIZE - STACK_SIZE;
}