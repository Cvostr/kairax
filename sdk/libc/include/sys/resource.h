#ifndef _SYS_RESOURCE_H
#define _SYS_RESOURCE_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#define RLIM_INFINITY (~0UL)

typedef unsigned long rlim_t;

struct rlimit {
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

#define RLIMIT_STACK	3
#define RLIMIT_NOFILE	7

int getrlimit(int resource, struct rlimit *rlim);

__END_DECLS

#endif