#ifndef _UN_H
#define _UN_H

#include <sys/cdefs.h>
#include "types.h"

__BEGIN_DECLS

#define LOCAL_PATH_MAX	108

struct sockaddr_un {
    sa_family_t sun_family;
    char sun_path[LOCAL_PATH_MAX];
};

__END_DECLS

#endif