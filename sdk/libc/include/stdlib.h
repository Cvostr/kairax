#ifndef _STDLIB_H
#define _STDLIB_H

#include <sys/types.h>
#include "stddef.h"

extern int atoi (const char *);
extern long atol (const char *);

extern char* itoa(int number, char* str, int base);

extern int abs (int __x);
extern long int labs (long int __x);
extern long long int llabs (long long int __x);

void *malloc(size_t nbytes);

void free(void *cp);

#endif