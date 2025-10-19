#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H

#include <string.h>
#include "time.h"
#include "signal.h"

__BEGIN_DECLS

#define FD_SETSIZE	1024
#define NFDBITS	    (8 * sizeof(unsigned long))

#define __FDS_IN_LONG   (FD_SETSIZE / FD_SETSIZE)
#define __FDSET_BYTE(d)	    ((d) / NFDBITS)
#define __MAKE_FDMASK(d)	(1UL << ((d) % NFDBITS))

typedef struct {
    unsigned long fds_bits [__FDS_IN_LONG];
} fd_set;

#define FD_ZERO(set)        (memset ((void*) (set), 0, sizeof(fd_set)))
#define FD_SET(d, set)	    ((set)->fds_bits[__FDSET_BYTE(d)] |= __MAKE_FDMASK(d))
#define FD_CLR(d, set)	    ((set)->fds_bits[__FDSET_BYTE(d)] &= ~__MAKE_FDMASK(d))
#define FD_ISSET(d, set)	(((set)->fds_bits[__FDSET_BYTE(d)] & __MAKE_FDMASK(d)) != 0)

int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) __THROW;

//int pselect(int n, fd_set* readfds, fd_set* writefds, fd_set* exceptfds,
//            const struct timespec *timeout, const sigset_t *sigmask) __THROW;

__END_DECLS

#endif
