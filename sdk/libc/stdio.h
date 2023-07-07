#ifndef _STDIO_H
#define _STDIO_H

#include "types.h"
#include "stddef.h"

#define EOF (-1)

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

#endif