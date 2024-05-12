#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "sys/socket.h"

typedef uint32_t in_addr_t;
struct in_addr {
    in_addr_t s_addr;
};

typedef uint16_t in_port_t;

struct sockaddr_in {
    
    sa_family_t sin_family;
    in_port_t sin_port;
    struct in_addr sin_addr;

    unsigned char sin_zero[sizeof (struct sockaddr)
			   - sizeof(sa_family_t)
			   - sizeof (in_port_t)
			   - sizeof (struct in_addr)];
};

extern uint32_t ntohl (uint32_t nlong);
extern uint16_t ntohs (uint16_t nshort);
extern uint32_t htonl (uint32_t hlong);
extern uint16_t htons (uint16_t hshort);

#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

#ifdef __cplusplus
}
#endif

#endif