#ifndef _UDP_H
#define _UDP_H

#include "kairax/types.h"
#include "net/net_buffer.h"
#include "ipc/socket.h"
#include "list/list.h"
#include "sync/spinlock.h"
#include "proc/blocker.h"

struct udp_packet {
    uint16_t    src_port;
    uint16_t    dst_port;
    uint16_t    len;
    uint16_t    checksum;   
    uint8_t     datagram[];
};

struct udp4_socket_data {

    struct nic* nic;
    uint16_t port;

    list_t rx_queue;
    spinlock_t rx_queue_lock;

    struct blocker blk;

    uint8_t allow_broadcast;
};

int udp_ip4_handle(struct net_buffer* nbuffer);
int udp_ip4_alloc_dynamic_port(struct socket* sock);

void udp_ip4_init();

uint16_t udp4_calc_checksum(uint32_t src, uint32_t dest, struct udp_packet* header, const unsigned char* payload, size_t payload_size);

int sock_udp4_create(struct socket* sock);
int sock_udp4_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);
ssize_t sock_udp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
int sock_udp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
int sock_udp4_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen);
int sock_udp4_close(struct socket* sock);

#endif