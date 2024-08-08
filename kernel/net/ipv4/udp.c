#include "udp.h"
#include "ipv4.h"
#include "kairax/string.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "kstdlib.h"

struct socket** udp4_bindings;

struct ip4_protocol ip4_udp_protocol = {
    .handler = udp_ip4_handle
};

void udp_ip4_handle(struct net_buffer* nbuffer)
{
    struct udp_packet* udphdr = (struct udp_packet*) nbuffer->cursor;
    nbuffer->transp_header = udphdr;

    nbuffer->payload = udphdr->datagram;
    net_buffer_shift(nbuffer, sizeof(struct udp_packet));

    uint16_t src_port = ntohs(udphdr->src_port);
    uint16_t dst_port = ntohs(udphdr->dst_port);

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
    printk("bind() to port %i\n", port);

    udp4_bindings[port] = sock;
    return 0;
}

int sock_udp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    printk("recvfrom\n");
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

    struct list_node* current_node = sock_data->rx_queue.head;
    struct net_buffer* cur_nbuffer = NULL;
    struct net_buffer* nbuffer = NULL;

    while (current_node != NULL) {
        cur_nbuffer = (struct net_buffer*) current_node->element;
        
        struct ip4_packet* ip4hdr = nbuffer->netw_header;
        struct udp_packet* udphdr = nbuffer->transp_header;

        if (ip4hdr->dst_ip == src_addr_in->sin_addr.s_addr && udphdr->src_port == src_addr_in->sin_port) {
            nbuffer = cur_nbuffer;
            break;
        }

        // Переход на следующий элемент
        current_node = current_node->next;
    }

    len = MIN(len, net_buffer_get_remain_len(nbuffer));

    memcpy(buf, nbuffer->cursor, len);

    net_buffer_shift(nbuffer, len);

    if (net_buffer_get_remain_len(nbuffer) < 1) 
    {
        net_buffer_close(nbuffer);
        list_remove(&sock_data->rx_queue, current_node);
        kfree(current_node);
    }

    release_spinlock(&sock_data->rx_queue_lock);
}

int sock_udp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    printk("sendto\n");
    return 0;
}

int sock_udp4_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen)
{
    printk("Setsockopt level:%i name:%i\n");
    return 0;
}