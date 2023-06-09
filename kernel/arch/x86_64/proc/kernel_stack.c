#include "kernel_stack.h"
#include "mem/kheap.h"
#include "cpu/msr.h"

kernel_stack_t kstack;

void init_kernel_stack()
{
    kstack.kernel_stack = kmalloc(4096);
    cpu_set_gs_base(&kstack);
}