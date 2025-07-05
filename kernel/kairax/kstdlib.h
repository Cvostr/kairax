#ifndef _KSTDLIB_H
#define _KSTDLIB_H

#include "types.h"

uint64_t align(uint64_t val, size_t alignment);

uint64_t align_down(uint64_t val, size_t alignment);

void seize_str(uint16_t* in, char* out);

void reverse(char *str);

char* itoa(int64 number, int base);

char* ulltoa(uint64_t number, int base);

long int strtol(const char *nptr, char **endptr, int base);

int atoi(const char *s);
long atol(const char *s);

int tolower(int c);

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#endif