#include "stdlib.h"

#define NUM_ATEXIT	32
typedef void (*atexit_func)(void);

static atexit_func __atexit_functions[NUM_ATEXIT];
static short __atexit_count = 0;

int atexit(atexit_func func)
{
    if (__atexit_count < NUM_ATEXIT) 
    {
        __atexit_functions[__atexit_count ++] = func;
        return 0;
    }

    return -1;
}

void __atexit_callall()
{
    int i = __atexit_count;
    while (i > 0)
    {
        __atexit_functions[--i]();
    }
}