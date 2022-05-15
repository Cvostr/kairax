#ifndef _THREAD_H
#define _THREAD_H

#include "stdint.h"

#define STACK_SIZE 1024

typedef struct PACKED {
    uint64_t        rbp;
    uint64_t        rbx;
    uint64_t        r12;
    uint64_t        r13;
    uint64_t        r14;
    uint64_t        r15;
    uint64_t        rbp2;
    uint64_t        ret;
} thread_stack_t;

typedef struct PACKED {
    char            name[32];
    uint64_t        thread_id;
    thread_stack_t* stack_ptr;
    int             status;
} thread_t;

thread_t* create_new_thread(void (*function)(void));

#endif