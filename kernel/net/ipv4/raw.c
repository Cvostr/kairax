#include "raw.h"
#include "net/route.h"
#include "ipv4.h"
#include "string.h"
#include "mem/kheap.h"
#include "stdio.h"
#include "kairax/kstdlib.h"

//#define RAW_LOG_SOCK_CLOSE

struct socket_prot_ops ipv4_raw_ops = {
    .create = sock_raw4_create,
    .recvfrom = sock_raw4_recvfrom,
    .sendto = sock_raw4_sendto,
    .setsockopt = sock_raw4_setsockopt,
    .close = sock_raw4_close
};

list_t raw_sockets;
spinlock_t raw_sockets_lock;

void raw4_accept_packet(int protocol, struct net_buffer* nbuffer)
{
    acquire_spinlock(&raw_sockets_lock);

    struct list_node* current = raw_sockets.head;
    while (current != NULL)
    {
        struct socket* sock = current->element;
        struct raw4_socket_data* sockdata = (struct raw4_socket_data*) sock->data; 

        if (sock->protocol == protocol)
        {
            //printk("Found rawsock\n");
            raw_ip4_put_to_rx_queue(sock, nbuffer);
        }

        current = current->next;
    }

    release_spinlock(&raw_sockets_lock);
}

void raw_ip4_put_to_rx_queue(struct socket* sock, struct net_buffer* nbuffer)
{
    struct raw4_socket_data* sockdata = (struct raw4_socket_data*) sock->data; 

    acquire_spinlock(&sockdata->rx_queue_lock);

    // Добавить буфер в очередь приема с увеличением количества ссылок
    net_buffer_acquire(nbuffer);
    list_add(&sockdata->rx_queue, nbuffer);
    
    release_spinlock(&sockdata->rx_queue_lock);

    // Разбудить ожидающих приема
    scheduler_wake(&sockdata->rx_blk, 1);
}

int sock_raw4_create (struct socket* sock)
{
    sock->data = kmalloc(sizeof(struct raw4_socket_data));
    memset(sock->data, 0, sizeof(struct raw4_socket_data));

    acquire_spinlock(&raw_sockets_lock);
    list_add(&raw_sockets, sock);
    inode_open((struct inode*) sock, 0);
    release_spinlock(&raw_sockets_lock);

    return 0;
}

int sock_raw4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    if (to == NULL)
    {
        return -EINVAL;
    }

    if (tolen != sizeof(struct sockaddr_in))
    {
        return -EINVAL;
    }

    struct sockaddr_in* dest_addr_in = (struct sockaddr_in*) to;

    struct raw4_socket_data* sock_data = (struct raw4_socket_data*) sock->data;
    // Маршрут назначения
    struct route4* route = route4_resolve(dest_addr_in->sin_addr.s_addr);
    if (route == NULL)
    {
        return -ENETUNREACH;
    }

    // Создать буфер
    struct net_buffer* resp = new_net_buffer_out(2048);
    resp->netdev = route->interface;
    net_buffer_acquire(resp);

    // Добавить сообщение в буфер
    net_buffer_add_front(resp, msg, len);
    
    // Отправить
    uint8_t ttl = sock_data->max_ttl != 0 ? sock_data->max_ttl : IPV4_DEFAULT_TTL; 
    int rc = ip4_send_ttl(resp, route, dest_addr_in->sin_addr.s_addr, sock->protocol, ttl);

    // освобождение памяти, выделенной под буфер
    net_buffer_free(resp);

    return rc;
}

ssize_t sock_raw4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    struct raw4_socket_data* sock_data = (struct raw4_socket_data*) sock->data;

    if (src_addr != NULL && *addrlen != sizeof(struct sockaddr_in))
    {
        return -EINVAL;
    }

    struct sockaddr_in* src_addr_in = (struct sockaddr_in*) src_addr;

    acquire_spinlock(&sock_data->rx_queue_lock);

    if (sock_data->rx_queue.head == NULL) 
    {
        // неблокирующее чтение
        if ((flags & MSG_DONTWAIT) == MSG_DONTWAIT)
        {
            return -EAGAIN;
        }

        release_spinlock(&sock_data->rx_queue_lock);
        scheduler_sleep_on(&sock_data->rx_blk);

        // Мы проснулись, но данные так и не пришли. Вероятно, нас разбудили сигналом
        if (sock_data->rx_queue.head == NULL) 
        {
            return -EINTR;
        }

        acquire_spinlock(&sock_data->rx_queue_lock);
    }

    struct net_buffer* nbuffer = list_dequeue(&sock_data->rx_queue);
    struct ip4_packet* ip4p = nbuffer->netw_header;

    release_spinlock(&sock_data->rx_queue_lock);

    // Чтение из net buffer в память процесса
    len = MIN(len, nbuffer->netw_packet_size);
    memcpy(buf, nbuffer->transp_header, len);

    // Освобождение буфера
    net_buffer_free(nbuffer);

    if (src_addr_in != NULL) 
    {
        // Передаем информацию об отправителе
        src_addr_in->sin_addr.s_addr = ip4p->src_ip;
    }

    return 0;
}

int sock_raw4_setsockopt_sol_socket(struct socket* sock, int optname, const void *optval, unsigned int optlen)
{
    struct raw4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;

    switch (optname) {
        case SO_BINDTODEVICE:
            //sock_data->nic = get_nic_by_name(optval);
            break;
    }

    return 0;
}

int sock_raw4_setsockopt_sol_IP(struct socket* sock, int optname, const void *optval, unsigned int optlen)
{
    struct raw4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;

    switch (optname) {
        case IP_TTL:
            if (optlen < 1)
                return -EINVAL;

            uint8_t val = *((uint8_t*) optval);
            sock_data->max_ttl = val;
            break;
    }

    return 0;
}


int sock_raw4_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen)
{
    //printk("RAW: setsockopt level:%i name:%i\n", level, optname);

    switch (level) {
        case SOL_SOCKET:
            return sock_raw4_setsockopt_sol_socket(sock, optname, optval, optlen);
        case SOL_IP:
            return sock_raw4_setsockopt_sol_IP(sock, optname, optval, optlen);

    };

    return 0;
}

void sock_raw4_drop_recv_buffer(struct raw4_socket_data* sock_data)
{
    acquire_spinlock(&sock_data->rx_queue_lock);
    struct net_buffer* current; 
    while ((current = list_dequeue(&sock_data->rx_queue)) != NULL)
    {
        net_buffer_free(current);
    }
    release_spinlock(&sock_data->rx_queue_lock);
}

int sock_raw4_close(struct socket* sock)
{
#ifdef RAW_LOG_SOCK_CLOSE
    printk("raw: close()\n");
#endif

    struct raw4_socket_data* sock_data = (struct udp4_socket_data*) sock->data;

    // удалить сокет из списка raw сокетов
    acquire_spinlock(&raw_sockets_lock);
    list_remove(&raw_sockets, sock);
    inode_close((struct inode*) sock);
    release_spinlock(&raw_sockets_lock);

    // Разбудить спящих
    scheduler_wake(&sock_data->rx_blk, INT_MAX);

    // Освободить память
    sock_raw4_drop_recv_buffer(sock_data);
    kfree(sock_data);

    return 0;
}