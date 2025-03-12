#include "udp.h"
#include "ipv4.h"
#include "kairax/string.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "kstdlib.h"
#include "net/route.h"

//#define UDP4_LOGGING
//#define UDP4_NO_LISTEN_LOG

#define UDP_DYNAMIC_PORT    49152
#define UDP_PORTS           65536

#define UDP_DEFAULT_RESPONSE_BUFLEN 4096

struct socket** udp4_bindings;

struct ip4_protocol ip4_udp_protocol = {
    .handler = udp_ip4_handle
};

int udp_ip4_alloc_dynamic_port(struct socket* sock)
{
    struct udp4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;
    for (int port = UDP_DYNAMIC_PORT; port < UDP_PORTS; port ++)
    {
        if (udp4_bindings[port] == NULL)
        {
            // порт с порядком байтов хоста
            sock_data->port = port;
            udp4_bindings[port] = sock;
            return 1;
        }
    }

    return 0;
}

void udp_ip4_handle(struct net_buffer* nbuffer)
{
    struct udp4_socket_data* sock_data = NULL;
    struct udp_packet* udphdr = (struct udp_packet*) nbuffer->transp_header;

    struct ip4_packet* ip4p = nbuffer->netw_header;

    nbuffer->payload = udphdr->datagram;
    nbuffer->payload_size = ntohs(udphdr->len) - sizeof(struct udp_packet);
    net_buffer_shift(nbuffer, sizeof(struct udp_packet));

    // Проверить контрольную сумму, если есть
    if (udphdr->checksum != 0) {
        uint16_t calculated = htons(udp4_calc_checksum(ip4p->src_ip, ip4p->dst_ip, udphdr, nbuffer->payload, nbuffer->payload_size));
        if (calculated != 0) {
            printk("UDP: Invalid Checksum!!!\n");
            return; // ???? todo: реализовать
        }
    }

    uint16_t src_port = ntohs(udphdr->src_port);
    uint16_t dst_port = ntohs(udphdr->dst_port);

#ifdef UDP4_LOGGING
    printk("UDP: src port: %i, dest port: %i, len: %i, payload_len: %i\n", src_port, dst_port, ntohs(udphdr->len), nbuffer->payload_size);
#endif

    struct socket* sock = udp4_bindings[dst_port];
    if (sock == NULL)
    {
#ifdef UDP4_NO_LISTEN_LOG
        printk("No listening socket");
#endif
        // ??? - реализовать
        return;
    }

    if (sock->data != NULL) 
    {
        sock_data = (struct udp4_socket_data*) sock->data;

        acquire_spinlock(&sock_data->rx_queue_lock);

        // Добавить буфер в очередь приема с увеличением количества ссылок
        net_buffer_acquire(nbuffer);
        list_add(&sock_data->rx_queue, nbuffer);
        
        release_spinlock(&sock_data->rx_queue_lock);

        // Будим ожидающих
        scheduler_wakeup_intrusive(&sock_data->blk.head, &sock_data->blk.tail, &sock_data->blk.lock, 1);
    }
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
    .close = sock_udp4_close
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

    struct udp4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;
    sock_data->port = port;
    udp4_bindings[port] = sock;

    return 0;
}

ssize_t sock_udp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    if (src_addr != NULL && *addrlen != sizeof(struct sockaddr_in))
    {
        return -EINVAL;
    }

    struct sockaddr_in* src_addr_in = (struct sockaddr_in*) src_addr;

    // TODO: проверить что сокет точно udp ip4

    struct udp4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;
    acquire_spinlock(&sock_data->rx_queue_lock);

    while (sock_data->rx_queue.head == NULL) {
        release_spinlock(&sock_data->rx_queue_lock);
        scheduler_sleep_on(&sock_data->blk);
        acquire_spinlock(&sock_data->rx_queue_lock);
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
    net_buffer_free(nbuffer);

    if (src_addr_in != NULL) {
        // Передаем информацию об отправителе
        src_addr_in->sin_addr.s_addr = ip4p->src_ip;
        src_addr_in->sin_port = udpp->src_port;
    }

    return len;
}

int sock_udp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    if (to == NULL)
    {
        return -EINVAL;
    }

    if (tolen != sizeof(struct sockaddr_in))
    {
        return -EINVAL;
    }

    struct udp4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;
    struct sockaddr_in* dest_addr_in = (struct sockaddr_in*) to;

    if (sock_data->port == 0)
    {
        // Если порт не назначен - назначить случайный ephemeral
        if (udp_ip4_alloc_dynamic_port(sock) == 0) {
            // порты закончились
            return -EADDRNOTAVAIL;
        }
    }

    // Маршрут назначения
    struct route4* route = route4_resolve(dest_addr_in->sin_addr.s_addr);
    if (route == NULL)
    {
        //printk("UDP: NO ROUTE!!!\n");
        return -ENETUNREACH;
    }

    struct net_buffer* resp = new_net_buffer_out(UDP_DEFAULT_RESPONSE_BUFLEN);
    resp->netdev = route->interface;
    net_buffer_acquire(resp);

    net_buffer_add_front(resp, msg, len);

    // Формируем UDP заголовок
    struct udp_packet udphdr;
    memset(&udphdr, 0, sizeof(struct udp_packet));
    udphdr.dst_port = dest_addr_in->sin_port;
    udphdr.src_port = htons(sock_data->port);
    udphdr.len = htons(sizeof(struct udp_packet) + len); 
    // Вычисление контрольной суммы
    udphdr.checksum = udp4_calc_checksum(resp->netdev->ipv4_addr, dest_addr_in->sin_addr.s_addr, &udphdr, msg, len);

    // Добавляем UDP заголовок к буферу
    net_buffer_add_front(resp, &udphdr, sizeof(struct udp_packet));

    int rc = ip4_send(resp, route, dest_addr_in->sin_addr.s_addr, IPV4_PROTOCOL_UDP);

    // освобождение памяти, выделенной под буфер
    net_buffer_free(resp);

    return rc;
}

int sock_udp4_setsockopt_sol_socket(struct socket* sock, int optname, const void *optval, unsigned int optlen)
{
    struct udp4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;

    switch (optname) {
        case SO_BINDTODEVICE:
            sock_data->nic = get_nic_by_name(optval);
            break;
    }

    return 0;
}

int sock_udp4_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen)
{
    printk("Setsockopt level:%i name:%i\n");
    switch (level) {
        case SOL_SOCKET:
            return sock_udp4_setsockopt_sol_socket(sock, optname, optval, optlen);
        
    };
    return 0;
}

int sock_udp4_close(struct socket* sock)
{
#ifdef UDP4_LOGGING
    printk("UDP: close()\n");
#endif
    struct udp4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;
    sock->data = NULL;

    if (udp4_bindings[sock_data->port] == sock) 
    {
        udp4_bindings[sock_data->port] = NULL;
    }

    acquire_spinlock(&sock_data->rx_queue_lock);
    
    struct net_buffer* nbuffer = NULL;
    while ((nbuffer = list_dequeue(&sock_data->rx_queue)) != NULL)
    {
        net_buffer_free(nbuffer);
    }
    
    release_spinlock(&sock_data->rx_queue_lock);

    kfree(sock_data);

    return 0;
}

uint16_t udp4_calc_checksum(uint32_t src, uint32_t dest, struct udp_packet* header, unsigned char* payload, size_t payload_size)
{
    uint16_t* data;
    int i = 0;
    uint32_t sum = 0;
    uint32_t header_sz = sizeof(struct udp_packet) / sizeof(uint16_t);

    // Псевдозаголовок

    sum += (src >> 16) & 0xFFFF;
    sum += (src) & 0xFFFF;

    sum += (dest >> 16) & 0xFFFF;
    sum += (dest) & 0xFFFF;

    sum += htons(IPPROTO_UDP);
    sum += htons(sizeof(struct udp_packet) + payload_size);

    // Заголовок
    data = (uint16_t*) header;
	for (i = 0; i < header_sz; i ++) {
		sum += (data[i]);
	}

    data = (uint16_t*) payload;
    while (payload_size > 1) {

        sum += *(data++);
        payload_size -= 2;
    }

    if (payload_size > 0) {
        sum += *((uint8_t*) data);
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (~sum) & 0xFFFF;
}