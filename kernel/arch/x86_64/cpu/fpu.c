#include "cpu.h"
#include "cpuid.h"
#include "kairax/types.h"

extern void x64_sse_enable();
extern void x64_osxsave_enable();
extern void x64_avx_enable();

#define SSE_AVAILABLE_BIT       (1UL << 25)
#define OSXSAVE_AVAILABLE_BIT   (1UL << 26)
#define AVX_AVAILABLE_BIT       (1UL << 28)

int fpu_init()
{
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, eax, ebx, ecx, edx);

    if ((edx & SSE_AVAILABLE_BIT) == SSE_AVAILABLE_BIT)
    {
        x64_sse_enable();
    } else {
        printk("SSE is not supported by CPU\n");
        return -1;
    }

    if ((ecx & OSXSAVE_AVAILABLE_BIT) == OSXSAVE_AVAILABLE_BIT)
    {
        x64_osxsave_enable();
    }

    if ((ecx & AVX_AVAILABLE_BIT) == AVX_AVAILABLE_BIT)
    {
        x64_avx_enable();
    }

    return 0;
}