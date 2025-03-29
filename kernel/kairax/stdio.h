#ifndef _STDIO_H
#define _STDIO_H

#include "types.h"
#include "stdarg.h"

#define printf printk

int printf_stdout(const char* __restrict, ...);
int printk(const char* __restrict, ...);
int sprintf(char* buffer, size_t buffersz, const char* format, ...);
int printf_generic(int (*f) (const char* str, size_t len, void* arg), size_t max, void* arg, const char* __restrict, va_list args);
int getch();
char* kfgets(char* buffer, size_t len);

#endif