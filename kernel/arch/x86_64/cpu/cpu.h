#ifndef _CPU_H
#define _CPU_H

int fpu_init();
int smp_init();

void fpu_context_init(void* ctx);

void fpu_save(void* buffer);
void fpu_restore(void* buffer);

#endif