#ifndef _UDP_H
#define _UDP_H

#include "kairax/types.h"

struct udp_packet {
    uint16_t    src_port;
    uint16_t    dst_port;
    uint16_t    len;
    uint16_t    checksum;   
    uint8_t     datagram[];
};

void udp_handle(unsigned char* data);

#endif