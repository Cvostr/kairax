#ifndef _IPV4_H
#define _IPV4_H

#include "kairax/types.h"
#include "net/net_buffer.h"

struct socket;

int ipv4_sock_create(struct socket* s, int type, int protocol);

#define IPV4_PROTOCOL_TCP   6
#define IPV4_PROTOCOL_UDP   17
#define IPV4_PROTOCOL_ICMP  1

union ip4uni {
    uint32_t val;
    uint8_t array[4];
};

struct ip4_protocol {
    int (*handler) (struct net_buffer*);
};

struct ip4_packet {
    uint8_t     version_ihl;
    uint8_t     tos;
    uint16_t    size;
    uint16_t    id;
    uint8_t     flags : 3;
    uint8_t     frag_offset_h : 5;
    uint8_t     frag_offset_l;
    uint8_t     ttl;
    uint8_t     protocol;
    uint16_t    header_checksum;
    uint32_t    src_ip;
    uint32_t    dst_ip;
} PACKED;

#define IP4_VERSION(x) ((x >> 4) & 0xF)
#define IP4_IHL(x) (x & 0xF)

uint16_t ip4_calculate_checksum(unsigned char* data, size_t len);

void ip4_handle_packet(struct net_buffer* nbuffer);

void ip4_register_protocol(struct ip4_protocol* protocol, int proto);

#endif