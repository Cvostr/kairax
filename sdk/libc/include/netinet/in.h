#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "sys/socket.h"

typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;
struct in_addr {
    in_addr_t s_addr;
};

struct in6_addr {
    union {
        uint8_t		    u6_addr8[16];
        uint16_t		u6_addr16[8];
        uint32_t		u6_addr32[4];
    } in6_u;
#define s6_addr			in6_u.u6_addr8
#define s6_addr16		in6_u.u6_addr16
#define s6_addr32		in6_u.u6_addr32
};

struct sockaddr_in {
    
    sa_family_t sin_family;
    in_port_t sin_port;
    struct in_addr sin_addr;

    unsigned char sin_zero[sizeof (struct sockaddr)
			   - sizeof(sa_family_t)
			   - sizeof (in_port_t)
			   - sizeof (struct in_addr)];
};

struct sockaddr_in6 {
    uint16_t		sin6_family;    
    uint16_t		sin6_port;      
    uint32_t		sin6_flowinfo;  
    struct in6_addr	sin6_addr;      
    uint32_t		sin6_scope_id;
};

extern uint32_t ntohl (uint32_t nlong);
extern uint16_t ntohs (uint16_t nshort);
extern uint32_t htonl (uint32_t hlong);
extern uint16_t htons (uint16_t hshort);

#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

#define	INADDR_ANY		    ((in_addr_t) 0x00000000) /* 0.0.0.0 */
#define	INADDR_BROADCAST	((unsigned long int) 0xffffffff)
#define	INADDR_LOOPBACK		0x7f000001	/* 127.0.0.1   */

#define IP_TOS		1
#define IP_TTL		2
#define IP_HDRINCL	3

#ifdef __cplusplus
}
#endif

#endif