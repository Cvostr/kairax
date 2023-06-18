#ifndef STDINT_H
#define STDINT_H

typedef unsigned int uint;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned char uint8;
typedef unsigned short uint16;

typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef int int32;
typedef char int8;
typedef short int16;
typedef long long int64;
typedef long long int64_t;

typedef unsigned long long uintptr_t; //64-bit system pointer

#include "stddef.h"

#define INT_MIN -2147483647 - 1
#define INT_MAX 2147483647
#define UINT32_MAX 0xFFFFFFFF

#endif