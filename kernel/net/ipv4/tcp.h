#ifndef _TCP_H
#define _TCP_H

#include "kairax/types.h"
#include "net/net_buffer.h"
#include "ipc/socket.h"

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

int tcp_ip4_handle(struct net_buffer* nbuffer);

void tcp_ip4_init();

int	sock_tcp4_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len);
int sock_tcp4_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);
int sock_tcp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
int sock_tcp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

#endif