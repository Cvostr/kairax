#ifndef _RAW_H
#define _RAW_H

#include "ipc/socket.h"
#include "proc/blocker.h"
#include "net/net_buffer.h"

#include "dev/type/net_device.h"

struct packet_raw_socket_data {

    struct nic* nic;

    list_t rx_queue;
    spinlock_t rx_queue_lock;

    struct blocker rx_blk;
};

struct packet_sockaddr_in {  
    sa_family_t sin_family;
    char ifname[NIC_NAME_LEN];
};

void packet_raw_accept_packet(int protocol, struct net_buffer* nbuffer);
void packet_raw_put_to_rx_queue(struct socket* sock, struct net_buffer* nbuffer);

int sock_packet_raw_create (struct socket* sock);
int sock_packet_raw_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);
int sock_packet_raw_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
ssize_t sock_packet_raw_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
int sock_packet_raw_close(struct socket* sock);
void sock_packet_raw_drop_recv_buffer(struct packet_raw_socket_data* sock_data);

#endif