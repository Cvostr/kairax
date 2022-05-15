#ifndef _STDIO_H
#define _STDIO_H

#include "stdint.h"

#define EOF (-1)

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

#endif
