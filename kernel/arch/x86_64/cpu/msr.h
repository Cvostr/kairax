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

#define IA32_GS_BASE            0xC0000101
#define IA32_KERNEL_GS_BASE     0xC0000102

void cpu_msr_get(uint32_t msr, uint64_t* value);

void cpu_msr_set(uint32_t msr, uint64_t value);

void cpu_enable_syscall_feature();

void cpu_set_syscall_params(uintptr_t entry_ip, uint16_t star_32_47, uint16_t star_48_63, uint64_t sfmask);

void cpu_set_kernel_gs_base(uintptr_t address);

void cpu_set_gs_base(uint16_t gs);

#endif