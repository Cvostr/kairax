#ifndef _STDIO_H
#define _STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "stddef.h"

#define EOF (-1)

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

typedef struct FILE; 

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);

#ifdef __cplusplus
}
#endif

#endif