#ifndef _ALIGN_H
#define _ALIGN_H

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

uint64_t align(uint64_t val, size_t alignment);
uint64_t align_down(uint64_t val, size_t alignment);

#endif