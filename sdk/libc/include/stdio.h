#ifndef _STDIO_H
#define _STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "stddef.h"
#include "stdarg.h"

#define EOF (-1)

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

struct _IO_FILE {
    int     _flags;
    int     _fileno;
    off_t   _off;
};

typedef struct _IO_FILE FILE;

extern FILE *stdin, *stdout, *stderr;

FILE *fopen(const char *restrict filename, const char *restrict mode);
FILE *fdopen(int fd, const char* restrict mode);

size_t fwrite(const void *restrict src, size_t size, size_t nmemb, FILE *restrict f);
size_t fread(void *restrict destv, size_t size, size_t nmemb, FILE *restrict f);

extern int vfprintf(FILE *stream, const char *format, va_list ap);
extern int snprintf(char *str, size_t size, const char *format, ...);
extern int vsnprintf(char* str, size_t size, const char *format, va_list arg_ptr);

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

#ifdef __cplusplus
}
#endif

#endif