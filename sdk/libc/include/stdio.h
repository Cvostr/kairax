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

struct IO_FILE;

typedef struct IO_FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *restrict filename, const char *restrict mode);
FILE *fdopen(int fd, const char* restrict mode);

size_t fwrite(const void *src, size_t size, size_t count, FILE *f);
size_t fread(void *destv, size_t size, size_t count, FILE *f);

int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);

int putchar(int);
int puts(const char*);

int fflush(FILE *stream);

extern int snprintf(char *str, size_t size, const char *format, ...);
extern int vsnprintf(char* str, size_t size, const char *format, va_list arg_ptr);

int fdprintf(int fd, const char *format, ...);
int vfdprintf(int fd, const char *format, va_list ap);

extern int fclose(FILE *stream);
extern int fflush(FILE *stream);

int remove(const char *pathname);
int rename(const char *oldpath, const char *newpath);

int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);

// ---- printf ------

int printf(const char* __restrict, ...);
int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list args);

int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list args);

// ---- scanf ------

int scanf(const char *format, ...);
int fscanf(FILE *stream, const char *format, ...);
int vfscanf(FILE *stream, const char *format, va_list args);

int sscanf(const char *str, const char *format, ...);
int vsscanf(const char *str, const char *format, va_list args);

int fgetc(FILE *f);


#ifdef __cplusplus
}
#endif

#endif