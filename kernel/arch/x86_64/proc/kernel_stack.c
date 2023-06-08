#include "kernel_stack.h"
#include "mem/kheap.h"
#include "cpu/msr.h"

kernel_stack_t kstack;

void init_kernel_stack()
{
    kstack.kernel_stack = kmalloc(4096);
    //printf("%i",  kstack.kernel_stack);
    cpu_set_gs_base(&kstack);
}