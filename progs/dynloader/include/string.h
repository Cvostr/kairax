#ifndef _STRING_H
#define _STRING_H

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

void *memset (void *__s, int __c, uint64_t __n);

void *memcpy (void *__restrict __dest, const void *__restrict __src, size_t __n);

int strcmp (const char *__s1, const char *__s2);

char *strcpy (char *__restrict __dest, const char *__restrict __src);

size_t strlen (const char *__s);

#endif