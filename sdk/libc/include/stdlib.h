#ifndef _STDLIB_H
#define _STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif


#include <sys/types.h>
#include "stddef.h"

extern int atoi (const char *);
extern long atol (const char *);

extern char* itoa(int number, char* str, int base);
extern char* ltoa(long number, char* str, int base);
extern char* lltoa(long long number, char* str, int base);

extern int abs (int __x);
extern long int labs (long int __x);
extern long long int llabs (long long int __x);

void *malloc(size_t nbytes);
void free(void *cp);
void *realloc(void *cp, size_t nbytes);
void *calloc(size_t num, size_t size);

extern void exit(int status);

#ifdef __cplusplus
}
#endif


#endif