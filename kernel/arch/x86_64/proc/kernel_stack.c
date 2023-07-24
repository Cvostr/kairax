#include "kernel_stack.h"
#include "mem/kheap.h"
#include "cpu/msr.h"
#include "stdio.h"


kernel_stack_t kstack;

void init_kernel_stack()
{
    kstack.kernel_stack = NULL;
    cpu_set_kernel_gs_base(&kstack);
    asm volatile("swapgs");
}

void set_kernel_stack(void* kstack_top)
{
    kstack.kernel_stack = kstack_top;
}

void* get_user_stack_ptr()
{
    return kstack.user_stack;
}

void set_user_stack_ptr(void* stack_ptr)
{
    kstack.user_stack = stack_ptr;
}