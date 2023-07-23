#ifndef _SCHED_CPU_H
#define _SCHED_CPU_H

#include "gdt.h"

struct cpu_local_x64 {
    gdt_entry_t* gdt;
    tss_t* tss;
    size_t gdt_size;
    int lapic_id;
    int id;
};

#endif
