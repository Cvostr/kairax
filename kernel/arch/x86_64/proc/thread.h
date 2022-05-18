#ifndef _THREAD_H
#define _THREAD_H

#include "stdint.h"
#include "process.h"
#include "cpu_ctx.h"

#define STACK_SIZE 1024

enum thread_state {
    THREAD_RUNNING          = 0,
    THREAD_INTERRUPTIBLE    = 1,
    THREAD_UNINTERRUPTIBLE  = 2,
    THREAD_ZOMBIE           = 4,
    THREAD_STOPPED          = 8,
    THREAD_SWAPPING         = 16,
    THREAD_EXCLUSIVE        = 32,
    THREAD_CREATED          = 64,
    THREAD_LOADING          = 128
};


typedef struct PACKED {
    char            name[32];
    uint64_t        thread_id;
    void*           stack_ptr;
    int             state;
    process_t*      process;
    cpu_context_t*  context;
} thread_t;

thread_t* create_new_thread(process_t* process, void (*function)(void));

#endif