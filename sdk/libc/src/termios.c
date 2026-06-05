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
        return ioctl(fd, TCSETS + optional_actions, (unsigned long long) termios_p);
    }

    errno = EINVAL;
    return -1;
}

int tcflush(int fd, int queue_selector)
{
    return ioctl(fd, TCFLSH, queue_selector);
}

void cfmakeraw(struct termios *t)
{
    t->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    t->c_oflag &= ~OPOST;
    t->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t->c_cflag &= ~(CSIZE);
    t->c_cflag |= CS8;
    t->c_cc[VMIN] = 1;
    t->c_cc[VTIME] = 0;
}

speed_t cfgetospeed(const struct termios *termios_p) 
{
    return ((termios_p->c_cflag & (CBAUD | CBAUDEX)));
}

speed_t cfgetispeed(const struct termios *termios_p) 
{
    return cfgetospeed(termios_p);
}

int cfsetospeed(struct termios *termios_p, speed_t speed)
{
    if ((speed & (speed_t)~CBAUD) != 0 && (speed < B57600 || speed > B460800)) {
        errno = EINVAL;
        return -1;
    }

    termios_p->c_cflag &= ~(CBAUD | CBAUDEX);
    termios_p->c_cflag |= speed;
    
    return 0;
}