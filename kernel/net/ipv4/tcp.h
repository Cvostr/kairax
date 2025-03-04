#ifndef _TCP_H
#define _TCP_H

#include "kairax/types.h"
#include "net/net_buffer.h"
#include "ipc/socket.h"
#include "ipv4.h"
#include "proc/blocker.h"

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
#define TCP_FLAG_SYNACK (TCP_FLAG_SYN | TCP_FLAG_ACK)


struct tcp4_socket_data {

    // порт, от имени которого будут уходить сообщения
    uint16_t            src_port;
    // порт, назначенный сокету локально
    // для клиента - случайно сгенерированный
    // для сервера - назначенный через bind()
    uint16_t            bound_port;
    // Адрес и порт пира в сетевой кодировке (Big Endian)
    struct sockaddr_in  addr;

    uint32_t sn;
    uint32_t ack;

    // Ожидаемые подключения
    struct net_buffer **backlog;
    int backlog_sz;
    int backlog_head;
    int backlog_tail;
    spinlock_t backlog_lock;
    list_t children;
    spinlock_t children_lock;
    
    // Указатель на listener сокет (сервер)
    struct tcp4_socket_data* listener;

    // очередь приема
    list_t rx_queue;
    spinlock_t rx_queue_lock;

    struct blocker backlog_blk;
    struct blocker rx_blk;
};

uint16_t tcp_ip4_calc_checksum(struct tcp_checksum_proto* prot, struct tcp_packet* header, size_t header_size, unsigned char* payload, size_t payload_size);

int tcp_ip4_handle(struct net_buffer* nbuffer);
int tcp_ip4_ack(struct tcp4_socket_data* sock_data);
void tcp_ip4_put_to_rx_queue(struct tcp4_socket_data* sock_data, struct net_buffer* nbuffer);
int tcp_ip4_alloc_dynamic_port(struct socket* sock);
void tcp_ip4_listener_add(struct tcp4_socket_data* listener, struct socket* client);
void tcp_ip4_listener_remove(struct tcp4_socket_data* listener, struct socket* client);
// addr и port в порядке байт сети (Big Endian)
struct socket* tcp_ip4_listener_get(struct tcp4_socket_data* listener, uint32_t addr, uint16_t port);

void tcp_ip4_init();


int sock_tcp4_create (struct socket* sock);
int	sock_tcp4_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len);
int	sock_tcp4_accept(struct socket *sock, struct socket **newsock, struct sockaddr *addr);
int sock_tcp4_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);
int sock_tcp4_listen(struct socket* sock, int backlog);
ssize_t sock_tcp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
int sock_tcp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
int sock_tcp4_close(struct socket* sock);
int sock_tcp4_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen);

#endif