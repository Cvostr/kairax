#ifndef _IOCTL_H_
#define _IOCTL_H_

#include "types.h"

int ioctl(int fd, uint64_t request, uint64_t arg);

#endif