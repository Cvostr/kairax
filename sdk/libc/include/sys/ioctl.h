#ifndef _IOCTL_H_
#define _IOCTL_H_

#include "types.h"
#include <stdint.h>

#define TCGETS  0x5401
#define TCSETS	0x5402

#define TIOCGPGRP	0x540F
#define TIOCSPGRP	0x5410

int ioctl(int fd, uint64_t request, uint64_t arg);

#endif