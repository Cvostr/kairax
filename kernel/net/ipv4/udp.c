#include "udp.h"
#include "ipv4.h"

struct ip4_protocol ip4_udp_protocol = {
    .handler = udp_ip4_handle
};

void udp_ip4_handle(struct net_buffer* nbuffer)
{
    struct udp_packet* udphdr = (struct udp_packet*) nbuffer->cursor;
    nbuffer->transp_header = udphdr;
}

void udp_ip4_init()
{
    ip4_register_protocol(&ip4_udp_protocol, IPV4_PROTOCOL_UDP);
}

struct socket_prot_ops ipv4_dgram_ops = {
    .bind = sock_udp4_bind,
    .recvfrom = sock_udp4_recvfrom,
    .sendto = sock_udp4_sendto
};

int sock_udp4_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen)
{

}

int sock_udp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{

}

int sock_udp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{

}