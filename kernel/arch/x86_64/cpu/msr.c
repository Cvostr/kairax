#include "msr.h"

void cpu_msr_get(uint32_t msr, uint64_t* value)
{
    uint32_t lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    *value = (uint64_t)hi << 32 | lo;
}

void cpu_msr_set(uint32_t msr, uint64_t value)
{
    uint32_t lo = value & UINT32_MAX;
    uint32_t hi = (value >> 32) & UINT32_MAX;
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

void cpu_enable_syscall_feature()
{
    uint64_t value;
    cpu_msr_get(MSR_EFER, &value);
    value |= 0x1;
    cpu_msr_set(MSR_EFER, value);
}

void cpu_set_syscall_params(uintptr_t entry_ip, uint16_t star_32_47, uint16_t star_48_63, uint64_t sfmask)
{
    cpu_msr_set(MSR_LSTAR, entry_ip);
    cpu_msr_set(MSR_SFMASK, sfmask);
    uint64_t star = ((uint64_t)star_32_47) << 32;
    
    star |= (((uint64_t)star_48_63) << 48); 
    cpu_msr_set(MSR_STAR, star);
}

void cpu_set_gs_base(uintptr_t address)
{
    cpu_msr_set(IA32_KERNEL_GS_BASE, address);
}