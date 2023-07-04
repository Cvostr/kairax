#include "thread.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "string.h"

int last_id = 0;

thread_t* new_thread(process_t* process)
{
    thread_t* thread    = (thread_t*)kmalloc(sizeof(thread_t));
    memset(thread, 0, sizeof(thread_t));
    thread->id          = last_id++;
    thread->process     = (process);
    return thread;
}