#include "termios.h"
#include "sys/ioctl.h"
#include "errno.h"

int tcgetattr(int fd, struct termios *termios_p)
{
    return ioctl(fd, TCGETS, (unsigned long long) termios_p);
}

int tcsetattr(int fd, int optional_actions, struct termios *termios_p)
{
    if (optional_actions < 3)
    {
        ioctl(fd, TCSETS + optional_actions, (unsigned long long) termios_p);
    }

    errno = EINVAL;
    return -1;
}