#ifndef _ARP_H
#define _ARP_H

#include "kairax/types.h"
#include "dev/type/net_device.h"
#include "net_buffer.h"

#define ARP_HTYPE_ETHERNET 0x0001

#define ARP_PTYPE_IPV4     0x0800 

#define ARP_OPER_REQ    0x0001
#define ARP_OPER_RESP   0x0002

struct arp_header {
    uint16_t    htype;
    uint16_t    ptype;
    uint8_t     hlen;
    uint8_t     plen;
    uint16_t    oper;

    uint8_t     sha[6];
    uint8_t     spa[4];
    uint8_t     tha[6];
    union {
        uint32_t    tpa;
        uint8_t     tpa_a[4];
    };
} PACKED;

void arp_handle_packet(struct net_buffer* nbuffer);

#endif