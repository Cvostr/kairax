#ifndef _THREAD_H
#define _THREAD_H

#include "stdint.h"

typedef struct PACKED {
    uint64_t thread_it;
    void* stack_ptr;
} thread_t;

thread_t* create_new_thread();

#endif