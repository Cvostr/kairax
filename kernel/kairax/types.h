#ifndef TYPES_H
#define TYPES_H

#define PACKED __attribute__((packed))

#include "stddef.h"

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

typedef unsigned long long uintptr_t;
typedef unsigned long long size_t;
typedef long long ssize_t;
typedef size_t loff_t;

typedef long long 			pid_t;
typedef unsigned int 		uid_t;
typedef unsigned int 		gid_t;
typedef unsigned long int 	ino_t;
typedef long int 			off_t;
typedef unsigned int 		mode_t;
typedef unsigned long int 	nlink_t;
typedef unsigned long 		dev_t;

typedef unsigned long int 	blkcnt_t;
typedef unsigned long int 	blksize_t;  

typedef long int time_t;

typedef signed long 		suseconds_t;
typedef signed long 		useconds_t;

typedef struct {
	int counter;
} atomic_t;

typedef struct {
	long counter;
} atomic_long_t;

#define INT_MIN 		(-2147483647 - 1)
#define INT_MAX 		2147483647
#define UINT16_MAX 		0xFFFF
#define UINT32_MAX 		0xFFFFFFFF
#define UINT64_MAX		(18446744073709551615ULL)

#endif