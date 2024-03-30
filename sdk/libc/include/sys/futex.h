#ifndef _FUTEX_H_
#define _FUTEX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "time.h"

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

int futex(int* addr, int op, int val, const struct timespec* timeout);

#ifdef __cplusplus
}
#endif

#endif