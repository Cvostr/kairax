#ifndef _POLL_H
#define _POLL_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#define POLLIN      0x001
#define POLLPRI     0x002
#define POLLOUT     0x004
#define POLLERR     0x008
#define POLLHUP     0x010
#define POLLNVAL    0x020
#define POLLRDNORM  0x040
#define POLLWRNORM	0x100
#define POLLRDHUP   0x2000

struct pollfd {
    int fd;
    short events;
    short revents;
};

typedef unsigned int nfds_t;

int poll(struct pollfd *ufds, nfds_t nfds, int timeout) __THROW;

__END_DECLS

#endif
