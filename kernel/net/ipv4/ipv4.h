#ifndef _IPV4_H
#define _IPV4_H

#include "kairax/types.h"
#include "net/net_buffer.h"
#include "kairax/in.h"

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

// ----ipv4 для userspace
typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;
struct in_addr {
    in_addr_t s_addr;
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
// ---

uint16_t ip4_calculate_checksum(unsigned char* data, size_t len);

void ip4_handle_packet(struct net_buffer* nbuffer);

void ip4_register_protocol(struct ip4_protocol* protocol, int proto);

int ip4_send(struct net_buffer* nbuffer, uint32_t dest, uint32_t src, uint8_t prot);

#endif