#ifndef _UDP_H
#define _UDP_H

#include "kairax/types.h"
#include "net/net_buffer.h"
#include "ipc/socket.h"

struct udp_packet {
    uint16_t    src_port;
    uint16_t    dst_port;
    uint16_t    len;
    uint16_t    checksum;   
    uint8_t     datagram[];
};

void udp_ip4_handle(struct net_buffer* nbuffer);

void udp_ip4_init();

int sock_udp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);

int sock_udp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

#endif