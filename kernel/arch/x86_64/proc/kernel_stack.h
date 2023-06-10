#ifndef KERNEL_STACK_H
#define KERNEL_STACK_H

#include "stddef.h"

typedef struct PACKED {
    char* user_stack;
    char* kernel_stack;
} kernel_stack_t;

void init_kernel_stack();

void set_kernel_stack(void* kstack_top);

#endif