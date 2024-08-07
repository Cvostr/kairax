#ifndef _STDLIB_H
#define _STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "stddef.h"

float strtof(const char *nptr, char **endptr);
double strtod(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);
long int strtol(const char *nptr, char **endptr, int base);
unsigned long int strtoul(const char *nptr, char **endptr, int base);

// Из расширений, поддерживаются по умолчанию
long long int strtoll(const char *nptr, char **endptr, int base);
unsigned long long int strtoull(const char *nptr, char **endptr, int base);
//

int __dtostr(double d, char *buf, unsigned int maxlen, unsigned int prec, unsigned int prec2, int flags);

extern int atoi (const char *);
extern long atol (const char *);
extern double atof(const char *nptr);

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

extern int rand(void);
extern int rand_r(unsigned int *seed);
extern void srand(unsigned int seed);

extern void exit(int status);
extern void abort();
extern int system(const char *command);

extern char *getenv(const char *name);
extern int setenv(const char *name, const char *value, int overwrite);
extern int unsetenv(const char *name);
extern int putenv(char *string);

#ifdef __cplusplus
}
#endif


#endif