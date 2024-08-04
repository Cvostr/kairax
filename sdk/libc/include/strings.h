#ifndef _STRINGS_H_
#define _STRINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stddef.h"

extern int strcasecmp(const char *__s1, const char *__s2);

extern int strncasecmp(const char *__s1, const char *__s2, size_t __n);

void bzero(void *s, size_t n);

#ifdef __cplusplus
}
#endif

#endif