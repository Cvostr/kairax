#ifndef _IOCTL_H_
#define _IOCTL_H_

#include "types.h"
#include <stdint.h>

#define TCGETS  0x5401
#define TCSETS	0x5402

int ioctl(int fd, uint64_t request, uint64_t arg);

#endif