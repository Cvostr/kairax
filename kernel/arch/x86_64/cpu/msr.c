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