#ifndef _ETH_H
#define _ETH_H

#include "kairax/types.h"

#define ETH_TYPE_ARP    0x0806

struct ethernet_frame {
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t type;
};

void eth_handle_frame(unsigned char* data, size_t len);

#endif