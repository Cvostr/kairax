#ifndef _STDLIB_H
#define _STDLIB_H

#include "stdint.h"

void reverse(char *str);

char* itoa(int64 number, int base);

char* ulltoa(uint64_t number, int base);

#endif