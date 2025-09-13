#ifndef _IOCTL_H_
#define _IOCTL_H_

#include "types.h"
#include <stdint.h>

#define TCGETS  0x5401
#define TCSETS	0x5402

#define TIOCGPGRP	0x540F
#define TIOCSPGRP	0x5410

#define TIOCGWINSZ  0x540E
#define TIOCSWINSZ  0x540F

#define TIOCNOTTY	0x5422

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

int ioctl(int fd, uint64_t request, uint64_t arg);

#endif