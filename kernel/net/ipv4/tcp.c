#include "tcp.h"
#include "ipv4.h"
#include "kairax/in.h"

struct ip4_protocol ip4_tcp_protocol = {
    .handler = tcp_ip4_handle
};

int tcp_ip4_handle(struct net_buffer* nbuffer)
{
    struct tcp_packet* tcp_packet = (struct tcp_packet*) nbuffer->cursor;
    nbuffer->transp_header = tcp_packet;

    uint16_t flags = ntohs(tcp_packet->hlen_flags);
    flags &= 0b111111111;

    printk("Src port: %i, dest port: %i, fl: %i\n", ntohs(tcp_packet->src_port), ntohs(tcp_packet->dst_port), flags);
}

struct socket_prot_ops ipv4_stream_ops = {
    .connect = sock_tcp4_connect,
    .accept = sock_tcp4_accept,
    .bind = sock_tcp4_bind,
    .recvfrom = sock_tcp4_recvfrom,
    .sendto = sock_tcp4_sendto
};

int	sock_tcp4_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len)
{

}

int	sock_tcp4_accept(struct socket *sock, struct socket **newsock, struct sockaddr *addr)
{
    return 0;
}

int sock_tcp4_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen)
{

}

int sock_tcp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{

}

int sock_tcp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{

}

void tcp_ip4_init()
{
    ip4_register_protocol(&ip4_tcp_protocol, IPV4_PROTOCOL_TCP);
}