#ifndef MSR_H
#define MSR_H

#include "stdint.h"

#define MSR_EFER        0xC0000080
#define MSR_APIC_BASE   0x1B

#define MSR_IA32_SYSENTER_CS    0x174
#define MSR_IA32_SYSENTER_ESP   0x175
#define MSR_IA32_SYSENTER_EIP   0x176

// SYSCALL / SYSRET
#define MSR_STAR                0xC0000081
#define MSR_LSTAR               0xC0000082
#define MSR_CSTAR               0xC0000083
#define MSR_SFMASK              0xC0000084

void cpu_msr_get(uint32_t msr, uint64_t* value);

void cpu_msr_set(uint32_t msr, uint64_t value);

void cpu_enable_syscall_feature();

void cpu_set_sysenter_params();

#endif