#ifndef _CPU_H
#define _CPU_H

#define INTERRUPT_VEC_TLB  0xFD
#define INTERRUPT_VEC_HLT  0xFE

int fpu_init();
int smp_init();

void fpu_context_init(void* ctx);

void fpu_save(void* buffer);
void fpu_restore(void* buffer);

void cpu_tlb_shootdown_ipi();

#endif