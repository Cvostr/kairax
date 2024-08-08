#include "tcp.h"
#include "ipv4.h"
#include "kairax/in.h"
#include "kairax/errors.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "string.h"

struct ip4_protocol ip4_tcp_protocol = {
    .handler = tcp_ip4_handle
};

struct socket* tcp4_bindings[65536];

int tcp_ip4_handle(struct net_buffer* nbuffer)
{
    struct tcp_packet* tcp_packet = (struct tcp_packet*) nbuffer->cursor;
    nbuffer->transp_header = tcp_packet;

    uint16_t flags = ntohs(tcp_packet->hlen_flags);
    flags &= 0b111111111;
    uint16_t src_port = ntohs(tcp_packet->src_port);
    uint16_t dst_port = ntohs(tcp_packet->dst_port);

    printk("Src port: %i, dest port: %i, fl: %i\n", src_port, dst_port, flags);

    struct socket* sock = tcp4_bindings[dst_port];
    if (sock == NULL)
    {
        // ??? - реализовать
        return -1;
    }

    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    if ((flags & TCP_FLAG_SYN) == TCP_FLAG_SYN) {

        if (sock->state != SOCKET_STATE_LISTEN) {
            // ? CONNREFUSED?
            return 1;
        }

        // запрос соединения
        if (sock_data->backlog_tail == sock_data->backlog_sz) {
            // ? CONNREFUSED?
            return 1;
        }

        struct ip4_packet* netw_header = nbuffer->netw_header;
        sock_data->backlog[sock_data->backlog_tail].sin_family = AF_INET;
        sock_data->backlog[sock_data->backlog_tail].sin_addr.s_addr = netw_header->src_ip;
        sock_data->backlog[sock_data->backlog_tail].sin_port = src_port;
        sock_data->backlog_tail++;

        scheduler_wakeup(sock_data->backlog, 1);
    }
}

struct socket_prot_ops ipv4_stream_ops = {
    .create = sock_tcp4_create,
    .connect = sock_tcp4_connect,
    .accept = sock_tcp4_accept,
    .bind = sock_tcp4_bind,
    .listen = sock_tcp4_listen,
    .recvfrom = sock_tcp4_recvfrom,
    .sendto = sock_tcp4_sendto
};

int sock_tcp4_create (struct socket* sock)
{
    sock->data = kmalloc(sizeof(struct tcp4_socket_data));
    memset(sock->data, 0, sizeof(struct tcp4_socket_data));
    return 0;
}

int	sock_tcp4_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len)
{
    return 0;   
}

int	sock_tcp4_accept(struct socket *sock, struct socket **newsock, struct sockaddr *addr)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    while (sock_data->backlog_head == sock_data->backlog_tail - 1) 
    {
        scheduler_sleep(sock_data->backlog, NULL);
    }

    struct sockaddr_in client;
    memcpy(&client, &sock_data->backlog[++sock_data->backlog_head], sizeof(struct sockaddr_in));

    union ip4uni src;
	src.val = client.sin_addr.s_addr; 
	printk("Accepted client: IP4 : %i.%i.%i.%i\n", src.array[0], src.array[1], src.array[2], src.array[3]);

    // TODO: завершить handshake
    //struct net_buffer* resp = new_net_buffer_out(4096, )

    return 0;
}

int sock_tcp4_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen)
{
    if (addr->sa_family != AF_INET || addrlen != sizeof(struct sockaddr_in))
    {
        return -EINVAL;
    }

    struct sockaddr_in* inetaddr = (struct sockaddr_in*) addr;

    uint16_t port = ntohs(inetaddr->sin_port);
    //printk("Binding to port %i\n", port);

    tcp4_bindings[port] = sock;

    return 0;
}

int sock_tcp4_listen(struct socket* sock, int backlog)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    sock_data->backlog_sz = backlog;
    sock_data->backlog_head = -1;
    sock_data->backlog_tail = 0;
    sock_data->backlog = kmalloc(sizeof(struct sockaddr_in) * backlog);

    sock->state = SOCKET_STATE_LISTEN;

    return 0;
}

int sock_tcp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    return 0;
}

int sock_tcp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    return 0;
}

void tcp_ip4_init()
{
    ip4_register_protocol(&ip4_tcp_protocol, IPV4_PROTOCOL_TCP);
}