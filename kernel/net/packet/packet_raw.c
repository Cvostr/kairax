#include "packet_raw.h"
#include "string.h"
#include "mem/kheap.h"
#include "stdio.h"
#include "kairax/kstdlib.h"
#include "proc/thread_scheduler.h"

//#define RAW_LOG_SOCK_CLOSE

struct socket_prot_ops packet_raw_ops = {
    .create = sock_packet_raw_create,
    .bind = sock_packet_raw_bind,
    .recvfrom = sock_packet_raw_recvfrom,
    .sendto = sock_packet_raw_sendto,
    .poll = sock_packet_raw_poll,
    .close = sock_packet_raw_close
};

list_t packet_raw_sockets;
spinlock_t packet_raw_sockets_lock;

void packet_raw_accept_packet(int protocol, struct net_buffer* nbuffer)
{
    if (packet_raw_sockets.head == NULL)
    {
        return;
    }

    acquire_spinlock(&packet_raw_sockets_lock);

    struct list_node* current = packet_raw_sockets.head;
    while (current != NULL)
    {
        struct socket* sock = current->element;
        struct packet_raw_socket_data* sockdata = (struct packet_raw_socket_data*) sock->data; 
        // Совпадает протокол и указатель на устройство
        if (sock->protocol == protocol && sockdata->nic == nbuffer->netdev)
        {
            packet_raw_put_to_rx_queue(sock, nbuffer);
        }

        current = current->next;
    }

    release_spinlock(&packet_raw_sockets_lock);
}

void packet_raw_put_to_rx_queue(struct socket* sock, struct net_buffer* nbuffer)
{
    struct packet_raw_socket_data* sockdata = (struct packet_raw_socket_data*) sock->data; 

    acquire_spinlock(&sockdata->rx_queue_lock);

    // Добавить буфер в очередь приема с увеличением количества ссылок
    net_buffer_acquire(nbuffer);
    list_add(&sockdata->rx_queue, nbuffer);
    
    release_spinlock(&sockdata->rx_queue_lock);

    // Разбудить ожидающих приема
    scheduler_wake(&sockdata->rx_blk, 1);
    // Будим наблюдающих
    poll_wakeall(&sockdata->poll_wq);
}

int sock_packet_raw_create (struct socket* sock)
{
    sock->data = kmalloc(sizeof(struct packet_raw_socket_data));
    if (sock->data == NULL)
    {
        return -ENOMEM;
    }

    memset(sock->data, 0, sizeof(struct packet_raw_socket_data));

    acquire_spinlock(&packet_raw_sockets_lock);
    list_add(&packet_raw_sockets, sock);
    acquire_socket(sock);
    release_spinlock(&packet_raw_sockets_lock);

    return 0;
}

int sock_packet_raw_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen)
{
    struct packet_sockaddr_in* packet_addr = addr;

    if (addr->sa_family != AF_PACKET || addrlen != sizeof(struct packet_sockaddr_in))
    {
        return -EINVAL;
    }

    struct packet_raw_socket_data* sockdata = (struct packet_raw_socket_data*) sock->data; 
    sockdata->nic = get_nic_by_name(packet_addr->ifname);

    if (sockdata->nic == NULL)
    {
        return -ENODEV;
    }

    return 0;
}

int sock_packet_raw_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    if (to != NULL || tolen != 0)
    {
        return -EINVAL;
    }

    struct packet_raw_socket_data* sock_data = (struct packet_raw_socket_data*) sock->data;  
    
    if (sock_data->nic == NULL)
    {
        return -ENODEV;
    }

    // Создать буфер
    struct net_buffer* resp = new_net_buffer_out(2048);
    resp->netdev = sock_data->nic;
    net_buffer_acquire(resp);

    // Добавить сообщение в буфер
    net_buffer_add_front(resp, msg, len);
    
    // Отправить
    int rc = sock_data->nic->tx(sock_data->nic, msg, len);

    // освобождение памяти, выделенной под буфер
    net_buffer_free(resp);

    return rc;    
}

ssize_t sock_packet_raw_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    struct packet_raw_socket_data* sock_data = (struct packet_raw_socket_data*) sock->data;

    // Проверка длины
    if (src_addr != NULL && *addrlen != sizeof(struct packet_sockaddr_in))
    {
        return -EINVAL;
    }

    struct packet_sockaddr_in* src_addr_in = (struct packet_sockaddr_in*) src_addr;

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

    release_spinlock(&sock_data->rx_queue_lock);

    // Чтение из net buffer в память процесса
    len = MIN(len, nbuffer->buffer_len);
    memcpy(buf, nbuffer->buffer, len);

    // Освобождение буфера
    net_buffer_free(nbuffer);

    if (src_addr_in != NULL) 
    {
        // Передаем информацию об отправителе
        src_addr_in->sin_family = AF_PACKET;
        strncpy(src_addr_in->ifname, nbuffer->netdev->name, NIC_NAME_LEN);
    }

    return 0;
}

short sock_packet_raw_poll(struct socket *sock, struct file *file, struct poll_ctl *pctl)
{
    short poll_mask = 0;
    struct packet_raw_socket_data* sock_data = (struct packet_raw_socket_data*) sock->data;

    poll_wait(pctl, file, &sock_data->poll_wq);

    if (sock_data->rx_queue.head != NULL)
        poll_mask |= (POLLIN | POLLRDNORM);

    // Пока что всегда разрешаем запись
    poll_mask |= (POLLOUT | POLLWRNORM);
    
    return poll_mask;
}

int sock_packet_raw_close(struct socket* sock)
{
    struct packet_raw_socket_data* sock_data = (struct packet_raw_socket_data*) sock->data;

    // удалить сокет из списка raw сокетов
    acquire_spinlock(&packet_raw_sockets_lock);
    list_remove(&packet_raw_sockets, sock);
    free_socket(sock);
    release_spinlock(&packet_raw_sockets_lock);

    // Разбудить спящих
    scheduler_wake(&sock_data->rx_blk, INT_MAX);

    // Освободить память
    sock_raw4_drop_recv_buffer(sock_data);
}

void sock_packet_raw_drop_recv_buffer(struct packet_raw_socket_data* sock_data)
{
    acquire_spinlock(&sock_data->rx_queue_lock);
    struct net_buffer* current; 
    while ((current = list_dequeue(&sock_data->rx_queue)) != NULL)
    {
        net_buffer_free(current);
    }
    release_spinlock(&sock_data->rx_queue_lock);
}