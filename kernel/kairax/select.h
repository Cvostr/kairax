#ifndef _SELECT_H
#define _SELECT_H

#define FD_SETSIZE	1024
#define NFDBITS	    (8 * sizeof(unsigned long))

#define __FDS_IN_LONG   (FD_SETSIZE / FD_SETSIZE)
#define __FDSET_BYTE(d)	    ((d) / NFDBITS)
#define __MAKE_FDMASK(d)	(1UL << ((d) % NFDBITS))

typedef struct {
    unsigned long fds_bits [__FDS_IN_LONG];
} fd_set;

#endif
