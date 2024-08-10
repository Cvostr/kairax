#ifndef _ETH_H
#define _ETH_H

#include "kairax/types.h"
#include "dev/device.h"
#include "net_buffer.h"

#define ETH_TYPE_ARP    0x0806
#define ETH_TYPE_IPV4   0x0800
#define ETH_TYPE_IPV6   0x86DD

struct ethernet_frame {
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t type;
    uint8_t payload[];
} PACKED;

void eth_handle_frame(struct net_buffer* nbuffer);

int eth_send_nbuffer(struct net_buffer* nbuffer, uint8_t* dest, int type);

#endif