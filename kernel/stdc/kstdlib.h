#ifndef _KSTDLIB_H
#define _KSTDLIB_H

#include "types.h"

uint64_t align(uint64_t val, size_t alignment);

uint64_t align_down(uint64_t val, size_t alignment);

void reverse(char *str);

char* itoa(int64 number, int base);

char* ulltoa(uint64_t number, int base);

#endif