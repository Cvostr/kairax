#ifndef _PTY_H
#define _PTY_H

#include <sys/cdefs.h>
#include <termios.h>
#include <sys/ioctl.h>

__BEGIN_DECLS

int openpty(int *amaster, int *aslave, char *name, struct termios *termp, struct winsize *winp) __THROW;

__END_DECLS

#endif