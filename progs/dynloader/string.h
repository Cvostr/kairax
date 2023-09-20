#ifndef _STRING_H
#define _STRING_H

#include <sys/types.h>
#include <stdint.h>

void *memset (void *__s, int __c, uint64_t __n);

int strcmp (const char *__s1, const char *__s2);

#endif