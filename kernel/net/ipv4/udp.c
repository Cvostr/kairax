#include "udp.h"
#include "ipv4.h"
#include "kairax/string.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "kstdlib.h"

//#define UDP4_LOGGING

#define UDP_DEFAULT_RESPONSE_BUFLEN 4096

struct socket** udp4_bindings;

struct ip4_protocol ip4_udp_protocol = {
    .handler = udp_ip4_handle
};

void udp_ip4_handle(struct net_buffer* nbuffer)
{
    struct udp_packet* udphdr = (struct udp_packet*) nbuffer->cursor;
    nbuffer->transp_header = udphdr;

    nbuffer->payload = udphdr->datagram;
    nbuffer->payload_size = ntohs(udphdr->len) - sizeof(struct udp_packet);
    net_buffer_shift(nbuffer, sizeof(struct udp_packet));

    uint16_t src_port = ntohs(udphdr->src_port);
    uint16_t dst_port = ntohs(udphdr->dst_port);

#ifdef UDP4_LOGGING
    printk("UDP: src port: %i, dest port: %i, len: %i, payload_len: %i\n", src_port, dst_port, ntohs(udphdr->len), nbuffer->payload_size);
#endif

    struct socket* sock = udp4_bindings[dst_port];
    if (sock == NULL)
    {
        // ??? - реализовать
        return;
    }

    struct udp4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;

    acquire_spinlock(&sock_data->rx_queue_lock);
    list_add(&sock_data->rx_queue, nbuffer);
    release_spinlock(&sock_data->rx_queue_lock);

    scheduler_wakeup(&sock_data->rx_queue, 1);
#ifdef UDP4_LOGGING
    printk("UDP: EXIT\n");
#endif
}

void udp_ip4_init()
{
    udp4_bindings = kmalloc(65536 * sizeof(struct socket*));
    ip4_register_protocol(&ip4_udp_protocol, IPV4_PROTOCOL_UDP);
}

struct socket_prot_ops ipv4_dgram_ops = {
    .create = sock_udp4_create,
    .bind = sock_udp4_bind,
    .recvfrom = sock_udp4_recvfrom,
    .sendto = sock_udp4_sendto,
    .setsockopt = sock_udp4_setsockopt,
};

int sock_udp4_create(struct socket* sock)
{
    sock->data = kmalloc(sizeof(struct udp4_socket_data));
    memset(sock->data, 0, sizeof(struct udp4_socket_data));
    return 0;
}

int sock_udp4_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen)
{
    if (addr->sa_family != AF_INET || addrlen != sizeof(struct sockaddr_in))
    {
        return -EINVAL;
    }

    struct sockaddr_in* inetaddr = (struct sockaddr_in*) addr;

    uint16_t port = ntohs(inetaddr->sin_port);

#ifdef UDP4_LOGGING
    printk("bind() to port %i\n", port);
#endif

    udp4_bindings[port] = sock;

    struct udp4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;
    sock_data->port = port;

    return 0;
}

ssize_t sock_udp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    if (*addrlen != sizeof(struct sockaddr_in))
    {
        return -EINVAL;
    }

    struct sockaddr_in* src_addr_in = (struct sockaddr_in*) src_addr;

    // TODO: проверить что сокет точно udp ip4

    struct udp4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;
    acquire_spinlock(&sock_data->rx_queue_lock);

    while (sock_data->rx_queue.size == 0) {
        scheduler_sleep(&sock_data->rx_queue, &sock_data->rx_queue_lock);
    }

    struct net_buffer* nbuffer = list_dequeue(&sock_data->rx_queue);
    struct udp_packet* udpp = nbuffer->transp_header;
    struct ip4_packet* ip4p = nbuffer->netw_header;

    release_spinlock(&sock_data->rx_queue_lock);

    // Чтение из net buffer в память процесса
    len = MIN(len, nbuffer->payload_size);
    memcpy(buf, nbuffer->payload, len);
    net_buffer_shift(nbuffer, len);

    // Освобождение буфера
    net_buffer_close(nbuffer);

    // Передаем информацию об отправителе
    src_addr_in->sin_addr.s_addr = ip4p->src_ip;
    src_addr_in->sin_port = udpp->src_port;

    return len;
}

int sock_udp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    struct udp4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;
    struct sockaddr_in* dest_addr_in = (struct sockaddr_in*) to;

    struct net_buffer* resp = new_net_buffer_out(UDP_DEFAULT_RESPONSE_BUFLEN);

    net_buffer_add_front(resp, msg, len);

    struct udp_packet udphdr;
    memset(&udphdr, 0, sizeof(struct udp_packet));
    udphdr.dst_port = dest_addr_in->sin_port;
    udphdr.src_port = htons(sock_data->port);
    udphdr.len = htons(sizeof(struct udp_packet) + len); 

    net_buffer_add_front(resp, &udphdr, sizeof(struct udp_packet));

    ip4_send(resp, dest_addr_in->sin_addr.s_addr, 0xFFFFFFFF, IPV4_PROTOCOL_UDP);
    net_buffer_close(resp);

    return 0;
}

int sock_udp4_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen)
{
    printk("Setsockopt level:%i name:%i\n");
    return 0;
}