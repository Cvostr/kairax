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
    uint16_t    hlen_flags;
    uint16_t    window_size;
    uint16_t    checksum;
    uint16_t    urgent_point;
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

struct tcp4_socket_data {

    // Ожидаемые подключения
    struct sockaddr_in *backlog;
    int backlog_sz;
    int backlog_head;
    int backlog_tail;
    spinlock_t backlog_lock;
};

int tcp_ip4_handle(struct net_buffer* nbuffer);

void tcp_ip4_init();

int sock_tcp4_create (struct socket* sock);
int	sock_tcp4_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len);
int	sock_tcp4_accept(struct socket *sock, struct socket **newsock, struct sockaddr *addr);
int sock_tcp4_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);
int sock_tcp4_listen(struct socket* sock, int backlog);
int sock_tcp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
int sock_tcp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

#endif