#ifndef _SCHED_CPU_H
#define _SCHED_CPU_H

#include "gdt.h"

struct cpu_local_x64 {
    // Указатели на стек пользователя и ядра. НЕ ПЕРЕМЕЩАТЬ!!!
    char* user_stack;
    char* kernel_stack;
    // НЕ ПЕРЕМЕЩАТЬ!!!

    gdt_entry_t* gdt;
    tss_t* tss;
    size_t gdt_size;
    int lapic_id;
    int id;
};

#endif
