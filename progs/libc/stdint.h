#ifndef _STDINT_H
#define _STDINT_H

#define INT8_MIN		(-128)
#define INT16_MIN		(-32767-1)
#define INT32_MIN		(-2147483647-1)
#define INT64_MIN		(-(9223372036854775807LL) - 1)

#define INT8_MAX		(127)
#define INT16_MAX		(32767)
#define INT32_MAX		(2147483647)
#define INT64_MAX		((9223372036854775807LL) - 1)

#define UINT8_MAX		(255)
#define UINT16_MAX		(65535)
#define UINT32_MAX		(4294967295U)
#define UINT64_MAX		(18446744073709551615ULL)

#define INTPTR_MIN		(-9223372036854775807L - 1)
#define INTPTR_MAX		(9223372036854775807L)
#define UINTPTR_MAX		(18446744073709551615UL)

typedef long int intptr_t;
typedef unsigned long int uintptr_t;

#endif