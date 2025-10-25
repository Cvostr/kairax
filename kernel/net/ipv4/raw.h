#ifndef _RAW_H
#define _RAW_H

#include "ipc/socket.h"
#include "proc/blocker.h"
#include "net/net_buffer.h"

struct raw4_socket_data {

    struct nic* nic;

    list_t rx_queue;
    spinlock_t rx_queue_lock;

    struct blocker rx_blk;
    struct poll_wait_queue poll_wq;

    uint8_t max_ttl;
};

void raw4_accept_packet(int protocol, struct net_buffer* nbuffer);
void raw_ip4_put_to_rx_queue(struct socket* sock, struct net_buffer* nbuffer);

int sock_raw4_create (struct socket* sock);
int sock_raw4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
ssize_t sock_raw4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
int sock_raw4_close(struct socket* sock);
short sock_raw4_poll(struct socket *sock, struct file *file, struct poll_ctl *pctl);
int sock_raw4_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen);
void sock_raw4_drop_recv_buffer(struct raw4_socket_data* sock_data);

#endif