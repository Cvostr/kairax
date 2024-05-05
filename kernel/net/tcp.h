#ifndef _TCP_H
#define _TCP_H

#include "kairax/types.h"

struct tcp_packet {
    uint16_t    src_port;
    uint16_t    dst_port;
    uint32_t    sn;
    uint32_t    ack;
    uint8_t     header_len; // + reserved
    uint8_t     flags;
    uint16_t    window_size;
    uint16_t    checksum;
    uint16_t    urgent_point;
};

#endif