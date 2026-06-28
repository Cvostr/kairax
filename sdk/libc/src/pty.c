#include <pty.h>
#include <stddef.h>
#include "errno.h"
#include "syscalls.h"

int openpty(int *amaster, int *aslave, char *name, struct termios *termp, struct winsize *winp)
{
    int rc = syscall_create_pty(amaster, aslave);
    if (rc < 0)
    {
        errno = -rc;
        return -1;
    }

    // name не используется

    if (termp != NULL)
    {
        tcsetattr(*aslave, TCSAFLUSH, termp);
    }

    if (winp != NULL)
    {
        ioctl(*aslave, TIOCSWINSZ, winp);
    }

    return 0;
}