#include "termios.h"
#include "sys/ioctl.h"

int tcgetattr(int fd, struct termios *termios_p)
{
    return ioctl(fd, TCGETS, (unsigned long long) termios_p);
}