#ifndef _NETINET_IF_ETHER_H
#define _NETINET_IF_ETHER_H

#include <inttypes.h>

#define	ETHER_ADDR_LEN	6
#define	ETHER_TYPE_LEN	2 

#define	ETHERTYPE_IP		0x0800
#define	ETHERTYPE_ARP		0x0806
#define	ETHERTYPE_VLAN		0x8100
#define	ETHERTYPE_IPV6		0x86dd
#define ETHERTYPE_LOOPBACK	0x9000

__BEGIN_DECLS

struct ether_header
{
    uint8_t  ether_dhost[ETHER_ADDR_LEN];
    uint8_t  ether_shost[ETHER_ADDR_LEN];
    uint16_t ether_type;
} __attribute__ ((__packed__));

__END_DECLS

#endif