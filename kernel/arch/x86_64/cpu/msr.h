#ifndef MSR_H
#define MSR_H

#include "stdint.h"

#define MSR_EFER        0xC0000080
#define MSR_APIC_BASE   0x1B

void cpu_msr_get(uint32_t msr, uint64_t* value);

void cpu_msr_set(uint32_t msr, uint64_t value);

void cpu_enable_syscall_feature();

#endif