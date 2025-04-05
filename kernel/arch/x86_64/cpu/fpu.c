#include "cpu_x64.h"
#include "cpuid.h"
#include "kairax/types.h"
#include "kairax/stdio.h"

extern void x64_sse_enable();
extern void x64_osxsave_enable();
extern void x64_avx_enable();

#define SSE_AVAILABLE_BIT       (1UL << 25)
#define OSXSAVE_AVAILABLE_BIT   (1UL << 26)
#define AVX_AVAILABLE_BIT       (1UL << 28)

struct fpu_context 
{
    uint16_t fcw; // FPU Control Word
    uint16_t fsw; // FPU Status Word
    uint8_t ftw;  // FPU Tag Words
    uint8_t zero; // Literally just contains a zero
    uint16_t fop; // FPU Opcode
    uint64_t rip;
    uint64_t rdp;
    uint32_t mxcsr;      // SSE Control Register
    uint32_t mxcsrMask;  // SSE Control Register Mask
    uint8_t st[8][16];   // FPU Registers, Last 6 bytes reserved
    uint8_t xmm[16][16]; // XMM Registers
} __attribute__((packed));


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

void fpu_context_init(void* ctx)
{
    struct fpu_context* fpuctx = (struct fpu_context*) ctx;
    fpuctx->mxcsr = 0x1f80; // Начальное MXCSR
    fpuctx->mxcsrMask = 0xffbf;
    fpuctx->fcw = 0x33f; // Начальное FPU Control Word 
}

void fpu_save(void* buffer)
{
    asm volatile("fxsave64 (%0) "::"r"(buffer));
}

void fpu_restore(void* buffer)
{   
    asm volatile("fxrstor64 (%0) "::"r"(buffer));
}