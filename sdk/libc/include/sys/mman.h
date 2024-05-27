#ifndef _MMAN_H_
#define _MMAN_H_

#include <sys/types.h>
#include "stddef.h"

#define	PROT_NONE	0x00
#define	PROT_READ	0x01
#define	PROT_WRITE	0x02
#define	PROT_EXEC	0x04

#define MAP_ANONYMOUS	0x20

void* mmap(void* addr, size_t length, int protection, int flags, int fd, int offset);
int munmap(void* addr, size_t length);

#endif