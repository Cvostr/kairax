#ifndef _NETINET_UDP_H
#define _NETINET_UDP_H

#include <inttypes.h>

__BEGIN_DECLS

struct udphdr {
    uint16_t source;
    uint16_t dest;	
    uint16_t len;	
    uint16_t check;
};

__END_DECLS

#endif
