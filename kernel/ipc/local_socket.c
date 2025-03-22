#include "local_socket.h"
#include "socket.h"
#include "mem/kheap.h"
#include "kairax/errors.h"
#include "list/list.h"
#include "kairax/string.h"
#include "fs/vfs/file.h"
#include "proc/thread_scheduler.h"
#include "stdio.h"

struct socket_family local_sock_family = {
    .family = AF_LOCAL,
    .create = local_sock_create
};

struct socket_prot_ops local_stream_ops = {
    .bind = sock_local_bind,
    .listen = sock_local_listen,
    .accept = sock_local_accept,
    .connect = sock_local_connect,
    .close = sock_local_close,
    .sendto = sock_local_sendto,
    .recvfrom = sock_local_recvfrom
};

struct socket_prot_ops local_dgram_ops = {
    .bind = sock_local_bind,
    .close = sock_local_close,
    .sendto = sock_local_sendto,
    .recvfrom = sock_local_recvfrom
};

//list_t local_sockets = {0};

int sock_local_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen)
{
    int rc = 0;

    if (addrlen < sizeof(sa_family_t)) {
        return -EINVAL;
    }

    if (addr->sa_family != AF_LOCAL) {
        return -EINVAL;
    }

    int namelen = addrlen - sizeof(sa_family_t);

    if (namelen < 1) {
        return -EINVAL;
    }

    struct sockaddr_un* addr_un = (struct sockaddr_un*) addr;
    struct local_socket* lsock = (struct local_socket*) sock->data;

    if (addr_un->sun_path[0] == 0 && (namelen - 1) > 0) 
    {
        // без ФС
        //strcpy(lsock->path, addr_un->sun_path + 1);
        //list_add(&local_sockets, lsock); 
        printk("Non FS sockets not implemented!\n");
        return -1;
    } else 
    {
        printk("Opening file %s\n", addr_un->sun_path);
        // Открыть файл в ядре
        struct file* file = file_open(NULL, addr_un->sun_path, 0, 0);
        if (file == NULL) 
        {
            // Файл не найден - создаем через mknod()
            rc = sys_mknodat(FD_CWD, addr_un->sun_path, 0755 | INODE_FLAG_SOCKET, 0);
            if (rc != 0) 
            {
                return rc;
            }

            // Открыть файл
            file = file_open(NULL, addr_un->sun_path, 0, 0);

            // Запонить указатель на сокет в private_data
            inode_open((struct inode*) sock, 0);
            file->inode->private_data = sock;

            // Закрыть файл
            file_close(file);

        } else {
            // Файл уже существует - значит занято
            return -EADDRINUSE;
        }
    }
    
    return 0;
}

int sock_local_listen(struct socket* sock, int backlog)
{
    struct local_socket* sock_data = (struct local_socket*) sock->data;
    sock_data->backlog_sz = backlog;

    // Принимающий подключения
    sock->state = SOCKET_STATE_LISTEN;
    return 0;
}

int	sock_local_accept (struct socket *sock, struct socket **newsock, struct sockaddr *addr)
{
    struct local_socket* sock_data = (struct local_socket*) sock->data;

    // Вытаскиваем из начала списка
    struct socket* client_sock = list_dequeue(&sock_data->backlog);

    if (client_sock == NULL)
    {
        // Ожидание подключений
        scheduler_sleep_on(&sock_data->backlog_blk);

        // Пробуем вытащить пакет
        acquire_spinlock(&sock_data->rx_queue_lock);
        client_sock = list_dequeue(&sock_data->backlog);
        release_spinlock(&sock_data->rx_queue_lock);

        if (client_sock == NULL)
        {
            // Мы проснулись, но подключений нет - выходим с EINTR
            return -EINTR;
        }
    }

    struct local_socket* client_sock_data = (struct local_socket*) client_sock->data;
    client_sock_data->connection_accepted = 1;
    scheduler_wake(&client_sock_data->connect_blk, 1);

    printk("ACCEPTED\n");

    return 0;
}

int	sock_local_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len)
{
    if (sock->state == SOCKET_STATE_UNCONNECTED)
    {
        return -EISCONN;
    }

    struct local_socket* sock_data = (struct local_socket*) sock->data;
    // Сброс флага
    sock_data->connection_accepted = 0;

    int rc = 0;
    struct sockaddr_un* addr_un = (struct sockaddr_un*) saddr;
    struct file* file = file_open(NULL, addr_un->sun_path, 0, 0);

    if (file == NULL)
    {
        return -ECONNREFUSED;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) == 0) {
        // Мы пытаемся открыть файл, который не является сокетом
        rc = -ECONNREFUSED;
        goto exit;
    }

    struct socket* server_sock = (struct socket*) file->inode->private_data;
    if (server_sock == NULL)
    {
        rc = -ECONNREFUSED;
        goto exit;
    }

    if (server_sock->state != SOCKET_STATE_LISTEN)
    {
        rc = -ECONNREFUSED;
        goto exit;
    }

    sock->state == SOCKET_STATE_CONNECTING;

    rc = socket_local_append_to_backlog(server_sock, sock);
    if (rc != 0)
        goto exit;

    // Засыпаем в ожидании ответа от сервера
    scheduler_sleep_on(&sock_data->connect_blk);

    if (sock_data->connection_accepted != 1)
    {
        // C той стороны не принято соединение, значит нас разбудили сигналом
        rc = -EINTR;
        goto exit;
    }

    sock->state == SOCKET_STATE_CONNECTED;

exit:
    file_close(file);
    return rc;
}

int socket_local_append_to_backlog(struct socket* server_sock, struct socket* sock)
{
    int rc = 0;
    struct local_socket* server_sock_data = (struct local_socket*) server_sock->data;

    acquire_spinlock(&server_sock_data->backlog_lock);
    if (list_size(&server_sock_data->backlog) < server_sock_data->backlog_sz) 
    {
        // Добавляем сокет к очереди подключений
        inode_open((struct inode*) sock, 0);
        list_add(&server_sock_data->backlog, sock);

        // Разбудить ожидающих подключений
        scheduler_wake(&server_sock_data->backlog_blk, 1);
    } else {
        rc = -ECONNREFUSED;
        goto exit;
    }

exit:
    release_spinlock(&server_sock_data->backlog_lock);
    return rc;
}

int sock_local_close(struct socket* sock)
{
    return 0;
}

ssize_t sock_local_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    return 0;
}

int sock_local_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    return 0;
}

int local_sock_create(struct socket* s, int type, int protocol) 
{
    switch(type) {
        case SOCK_STREAM:
            s->ops = &local_stream_ops;
            break;
        case SOCK_RAW:
        case SOCK_DGRAM:
            s->ops = &local_dgram_ops;
            break;
    }

    s->data = new_local_socket(type);
    if (s->data == NULL)
    {
        return -ENOMEM;
    }

    return 0;
}

struct socket_data* new_local_socket(int type)
{
    struct socket_data* sock = kmalloc(sizeof(struct local_socket));
    if (sock == NULL)
    {
        return sock;
    }
    
    memset(sock, 0, sizeof(struct local_socket));

    return sock;
}

void local_sock_init() 
{
    register_sock_family(&local_sock_family);
}