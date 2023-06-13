#include "kernel_stack.h"
#include "mem/kheap.h"
#include "cpu/msr.h"
#include "stdio.h"

kernel_stack_t kstack;

void init_kernel_stack()
{
    kstack.kernel_stack = NULL;//(char*)kmalloc(4096) + 4096;
    cpu_set_kernel_gs_base(&kstack);
}

void set_kernel_stack(void* kstack_top)
{
    kstack.kernel_stack = kstack_top;
}