#ifndef _TCP_H
#define _TCP_H

#include "kairax/types.h"
#include "net/net_buffer.h"
#include "ipc/socket.h"
#include "ipv4.h"

struct tcp_packet {
    uint16_t    src_port;
    uint16_t    dst_port;
    uint32_t    sn;
    uint32_t    ack;
    uint16_t    hlen_flags;
    uint16_t    window_size;
    uint16_t    checksum;
    uint16_t    urgent_point;
};

struct tcp_checksum_proto {
    uint32_t    src;
    uint32_t    dest;
    uint8_t     zero;
    uint8_t     prot;
    uint16_t    len;
};

#define TCP_FLAG_NULL   0
#define TCP_FLAG_FIN    1
#define TCP_FLAG_SYN    2
#define TCP_FLAG_RST    4
#define TCP_FLAG_PSH    8
#define TCP_FLAG_ACK    16
#define TCP_FLAG_URG    32
#define TCP_FLAG_ECE    64
#define TCP_FLAG_CWR    128
#define TCP_FLAG_NS     256


struct tcp4_socket_data {

    struct sockaddr_in addr;

    uint32_t sn;
    uint32_t ack;

    // Ожидаемые подключения
    struct net_buffer **backlog;
    int backlog_sz;
    int backlog_head;
    int backlog_tail;
    spinlock_t backlog_lock;
};

uint16_t tcp_ip4_calc_checksum(struct tcp_checksum_proto* prot, struct tcp_packet* header, size_t header_size, unsigned char* payload, size_t payload_size);

int tcp_ip4_handle(struct net_buffer* nbuffer);

void tcp_ip4_init();

int sock_tcp4_create (struct socket* sock);
int	sock_tcp4_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len);
int	sock_tcp4_accept(struct socket *sock, struct socket **newsock, struct sockaddr *addr);
int sock_tcp4_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);
int sock_tcp4_listen(struct socket* sock, int backlog);
ssize_t sock_tcp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
int sock_tcp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

#endif