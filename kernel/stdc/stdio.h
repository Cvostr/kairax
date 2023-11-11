#ifndef _STDIO_H
#define _STDIO_H

#include "types.h"
#include "stdarg.h"

int printf_stdout(const char* __restrict, ...);
int printf(const char* __restrict, ...);
int printf_generic(int (*f) (const char* str, size_t len), const char* __restrict, va_list args);
int getch();
char* kfgets(char* buffer, size_t len);

#endif