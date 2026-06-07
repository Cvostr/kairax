#ifndef _SCHED_H
#define _SCHED_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

int sched_yield (void);

int getcpu(unsigned int *cpu, unsigned int *node);

int sched_getcpu(void);

#ifdef __cplusplus
}
#endif

#endif