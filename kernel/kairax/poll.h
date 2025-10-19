#ifndef _POLL_H
#define _POLL_H

#define POLLIN      0x001
#define POLLPRI     0x002
#define POLLOUT     0x004
#define POLLERR     0x008
#define POLLHUP     0x010
#define POLLNVAL    0x020

struct pollfd {
    int fd;
    short events;
    short revents;
};

typedef unsigned int nfds_t;

#endif
