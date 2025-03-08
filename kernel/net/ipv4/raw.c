#include "raw.h"
#include "net/route.h"
#include "ipv4.h"
#include "string.h"
#include "mem/kheap.h"
#include "stdio.h"

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

    struct socket* result = NULL;
    struct list_node* current = raw_sockets.head;
    while (current != NULL)
    {
        struct socket* sock = current->element;
        struct raw4_socket_data* sockdata = (struct raw4_socket_data*) sock->data; 

        if (sock->protocol == protocol)
        {
            printk("Found rawsock\n");
        }
    }

    release_spinlock(&raw_sockets_lock);
}

int sock_raw4_create (struct socket* sock)
{
    sock->data = kmalloc(sizeof(struct raw4_socket_data));
    memset(sock->data, 0, sizeof(struct raw4_socket_data));
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

    // Маршрут назначения
    struct route4* route = route4_resolve(dest_addr_in->sin_addr.s_addr);
    if (route == NULL)
    {
        printk("RAW: NO ROUTE!!!\n");
        return -ENETUNREACH;
    }

    // Создать буфер
    struct net_buffer* resp = new_net_buffer_out(2048);
    resp->netdev = route->interface;
    net_buffer_acquire(resp);

    // Добавить сообщение в буфер
    net_buffer_add_front(resp, msg, len);
    
    // Отправить
    int rc = ip4_send(resp, route, dest_addr_in->sin_addr.s_addr, sock->protocol);

    // освобождение памяти, выделенной под буфер
    net_buffer_free(resp);

    return rc;
}

ssize_t sock_raw4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    struct raw4_socket_data* sockdata = (struct raw4_socket_data*) sock->data;
    return 0;
}

int sock_raw4_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen)
{
    printk("RAW: setsockopt\n");
    return 0;
}

int sock_raw4_close(struct socket* sock)
{
    return 0;
}