#include "local_socket.h"
#include "socket.h"
#include "mem/kheap.h"
#include "kairax/errors.h"
#include "list/list.h"
#include "kairax/string.h"
#include "fs/vfs/file.h"
#include "proc/thread_scheduler.h"
#include "stdio.h"
#include "kairax/kstdlib.h"

#include "proc/syscalls.h"

//#define LOCAL_SOCK_LOG_CLOSE
//#define LOCAL_SOCK_LOG_BIND

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
    .poll = sock_local_poll,
    .getsockname = sock_local_getsockname,
    .getpeername = sock_local_getpeername,
    .sendto = sock_local_sendto,
    .recvfrom = sock_local_recvfrom_stream
};

struct socket_prot_ops local_dgram_ops = {
    .bind = sock_local_bind,
    .close = sock_local_close,
    .sendto = sock_local_sendto_dgram,
    .recvfrom = sock_local_recvfrom_dgram
};

//list_t local_sockets = {0};

int sock_local_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen)
{
    int rc = 0;
    struct file* file = NULL;

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
#ifdef LOCAL_SOCK_LOG_BIND
        printk("Creating socket file %s\n", addr_un->sun_path);
#endif
        // Пробуем создать файл через mknodat
        rc = sys_mknodat(FD_CWD, addr_un->sun_path, 0755 | INODE_FLAG_SOCKET, 0);
        if (rc == -EEXIST)
        {
            return -EADDRINUSE;
        } 
        else if (rc != 0) 
        {
            return rc;
        }

        // Открыть файл
        rc = file_open_ex(NULL, addr_un->sun_path, 0, 0, &file);
        if (file == NULL)
        {
            return rc;
        }

        // Запомнить указатель на сокет в private_data
        acquire_socket(sock);
        file->inode->private_data = sock;

        // Сохранить указатель на inode в сокет
        inode_open(file->inode, 0);
        lsock->bound_inode = file->inode;

        // Закрыть файл
        file_close(file);
    }

    lsock->address_len = addrlen;
    // Сохраняем адрес в структуре сокета
    lsock->address = kmalloc(addrlen);
    memcpy(lsock->address, addr, addrlen);
    
    return 0;
}

int sock_local_listen(struct socket* sock, int backlog)
{
    struct local_socket* sock_data = (struct local_socket*) sock->data;

    if (sock->state != SOCKET_STATE_UNCONNECTED)
        return -EINVAL;

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

    // Создание сокета клиента
    struct socket* client_pair_sock = new_socket();
    local_sock_create(client_pair_sock, sock->type, sock->protocol);
    client_pair_sock->state = SOCKET_STATE_CONNECTED;
    struct local_socket* client_pair_sock_data = (struct local_socket*) client_pair_sock->data;
    // Установка пира для клиентского сокета со стороны сервера
    client_pair_sock_data->peer = client_sock;
    acquire_socket(client_sock);

    // Установить пира для клиента
    client_sock_data->peer = client_pair_sock;
    acquire_socket(client_pair_sock);
    // Разбудить поток, ожидающий подключения
    client_sock_data->connection_accepted = 1;
    scheduler_wake(&client_sock_data->connect_blk, 1);

    *newsock = client_pair_sock;

    // Уменьшить счетчик ссылок, т.к мы вытащили иноду из очереди
    free_socket(client_sock);

    return 0;
}

int	sock_local_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len)
{
    // Проверим, что размер адреса минимум sa_family_t
    if (sockaddr_len < sizeof(sa_family_t)) 
    {
        return -EINVAL;
    }

    // Проверим значение sa_family
    if (saddr->sa_family != AF_LOCAL) 
    {
        return -EINVAL;
    }

    if (sock->state != SOCKET_STATE_UNCONNECTED)
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

    sock->state = SOCKET_STATE_CONNECTED;

exit:
    file_close(file);
    return rc;
}

int sock_local_pair(struct socket* sock1, struct socket* sock2) 
{
    struct local_socket* sock1_data = (struct local_socket*) sock1->data;
    struct local_socket* sock2_data = (struct local_socket*) sock2->data;

    // Установка пира для первого сокета
    sock1_data->peer = sock2;
    acquire_socket(sock2);

    // Установить пира для второго сокета
    sock2_data->peer = sock1;
    acquire_socket(sock1);

    sock1->state = SOCKET_STATE_CONNECTED;
    sock2->state = SOCKET_STATE_CONNECTED;

    return 0;
}

int socket_local_append_to_backlog(struct socket* server_sock, struct socket* sock)
{
    int rc = 0;
    struct local_socket* server_sock_data = (struct local_socket*) server_sock->data;

    acquire_spinlock(&server_sock_data->backlog_lock);
    if (list_size(&server_sock_data->backlog) < server_sock_data->backlog_sz) 
    {
        // Добавляем сокет к очереди подключений
        acquire_socket(sock);
        list_add(&server_sock_data->backlog, sock);

        // Разбудить ожидающих подключений
        scheduler_wake(&server_sock_data->backlog_blk, 1);

        // Будим наблюдающих за сокетом сервера
        poll_wakeall(&server_sock_data->poll_wq);
    } else {
        rc = -ECONNREFUSED;
        goto exit;
    }

exit:
    release_spinlock(&server_sock_data->backlog_lock);
    return rc;
}

int sock_local_getsockname(struct socket *sock, struct sockaddr *name, socklen_t *namelen)
{
    sa_family_t default_family = AF_LOCAL;
    struct local_socket* sock_data = (struct local_socket*) sock->data;

    if (sock_data->address == NULL || sock_data->address_len == 0)
    {
        // для сокета не вызывался bind().
        // 
        return 0;
    }

    // найдем минимальную доступную длину
    socklen_t minlen = MIN(sock_data->address_len, *namelen);

    // копируем
    memcpy(name, sock_data->address, minlen);

    // не забываем записать кол-во скопированных байт
    if (minlen != *namelen)
    {
        *namelen = minlen;
    }

    return 0;
}

int sock_local_getpeername(struct socket *sock, struct sockaddr *addr, socklen_t *addrlen)
{
    struct local_socket* sock_data = (struct local_socket*) sock->data;
    struct local_socket* peer_sock_data;

    // Проверим, что сокет подключен
    if (sock->state != SOCKET_STATE_CONNECTED)
    {
        return -ENOTCONN;
    }

    if (sock_data->peer == NULL || sock_data->peer_disconnected == TRUE)
    {
        return -ENOTCONN;
    }

    // получить структуру пира
    peer_sock_data = sock_data->peer->data;
    
    if (peer_sock_data->address == NULL || peer_sock_data->address_len == 0)
    {
        // для сокета-пира не вызывался bind(). просто ничего не делаем
        return 0;
    }

    // найдем минимальную доступную длину
    socklen_t minlen = MIN(peer_sock_data->address_len, *addrlen);

    // копируем
    memcpy(addr, sock_data->address, minlen);

    // не забываем записать кол-во скопированных байт
    if (minlen != *addrlen)
    {
        *addrlen = minlen;
    }

    return 0;
}

int sock_local_close(struct socket* sock)
{
#ifdef LOCAL_SOCK_LOG_CLOSE
    printk("Local sock: close()\n");
#endif

    struct local_socket* sock_data = (struct local_socket*) sock->data;

    struct socket* peer = sock_data->peer;
    if (peer != NULL)
    {
        // Отсоединяем пира
        struct local_socket* peer_data = (struct local_socket*) peer->data;
        peer_data->peer_disconnected = 1;
        
        sock_data->peer = NULL;
        free_socket(peer);

        peer_data->peer = NULL;
        free_socket(sock);

        peer->state = SOCKET_STATE_UNCONNECTED;

        // Разбудить ожидающих приема данных от пира
        scheduler_wake(&peer_data->rx_blk, INT_MAX);

        // Будим наблюдающих за сокетом пира
        poll_wakeall(&peer_data->poll_wq);
    }

    // Очистить остатки буфера приема
    acquire_spinlock(&sock_data->rx_queue_lock);
    struct local_sock_bucket* current; 
    while ((current = list_dequeue(&sock_data->rx_queue)) != NULL)
    {
        kfree(current);
    }
    release_spinlock(&sock_data->rx_queue_lock);

    // Освободить память под inode файла сокета
    if (sock_data->bound_inode != NULL)
    {
        free_socket(sock_data->bound_inode->private_data);
        sock_data->bound_inode->private_data = NULL;

        inode_close(sock_data->bound_inode);
    }

    if (sock->state == SOCKET_STATE_LISTEN)
    {
        // В очереди на прием соединения могут остаться сокеты
        acquire_spinlock(&sock_data->backlog_lock);
        struct socket* current_peer; 
        while ((current_peer = list_dequeue(&sock_data->backlog)) != NULL)
        {
            struct local_socket* peer_sock_data = (struct local_socket*) current_peer->data;
            // Будим поток, ожидающий подключения
            scheduler_wake(&peer_sock_data->connect_blk, INT_MAX);
            // Снижаем счетчик ссылок на сокет
            free_socket(current_peer);
        }

        release_spinlock(&sock_data->backlog_lock);
    }

    sock->state = SOCKET_STATE_UNCONNECTED;

    // Если прибинжен адрес, то его надо освободить
    if (sock_data->address)
    {
        kfree(sock_data->address);
        sock_data->address = NULL;
    }

    return 0;
}

int sock_local_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    if (sock->state != SOCKET_STATE_CONNECTED)
    {
        return -ENOTCONN;
    }

    struct local_socket* sock_data = (struct local_socket*) sock->data;
    // Выделим память сразу и под заголовок и под данные
    unsigned char* bucket = kmalloc(sizeof(struct local_sock_bucket) + len);

    struct local_sock_bucket* header = (struct local_sock_bucket*) bucket;
    unsigned char* data = (header + 1);

    memcpy(data, msg, len);
    header->size = len;
    header->offset = 0;

    // Добавить сообщение в очередь приема пира
    struct socket* peer = sock_data->peer;
    struct local_socket* peer_data = (struct local_socket*) peer->data;
    
    acquire_spinlock(&peer_data->rx_queue_lock);
    list_add(&peer_data->rx_queue, header);
    peer_data->rx_available += len;
    release_spinlock(&peer_data->rx_queue_lock);

    // Разбудить одного ожидающего приема
    scheduler_wake(&peer_data->rx_blk, 1);

    // Будим наблюдающих за сокетом пира
    poll_wakeall(&peer_data->poll_wq);

    return 0;
}

short sock_local_poll(struct socket *sock, struct file *file, struct poll_ctl *nctl)
{
    short poll_mask = 0;
    struct local_socket* sock_data = (struct local_socket*) sock->data;

    poll_wait(nctl, file, &sock_data->poll_wq);

    if ((sock->state == SOCKET_STATE_LISTEN) && (sock_data->backlog.head != NULL))
        return POLLIN | POLLRDNORM;

    // Пир закрыл свой сокет
    if (sock_data->peer_disconnected == TRUE)
        poll_mask |= POLLHUP;

    // очередь приема не пуста?
    if (list_size(&sock_data->rx_queue) > 0)
        poll_mask |= (POLLIN | POLLRDNORM);

    // Пока что запись возможна, когда сокет соединен
    if (sock->state == SOCKET_STATE_CONNECTED)
        poll_mask |= (POLLOUT | POLLWRNORM);
    
    return poll_mask;
}

ssize_t sock_local_recvfrom_stream(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    ssize_t readed = 0;

    if (sock->state != SOCKET_STATE_CONNECTED)
    {
        return -ENOTCONN;
    }

    struct local_socket* sock_data = (struct local_socket*) sock->data;
    struct local_sock_bucket* rcv_buffer = list_head(&sock_data->rx_queue);

    if (rcv_buffer == NULL) 
    {
        // неблокирующее чтение
        if ((flags & MSG_DONTWAIT) == MSG_DONTWAIT)
        {
            return -EAGAIN;
        }

        if (rcv_buffer == NULL) 
        {
            // Ожидание пакета
            scheduler_sleep_on(&sock_data->rx_blk);

            // Пробуем получить первый пакет
            rcv_buffer = list_head(&sock_data->rx_queue);

            if (rcv_buffer == NULL)
            {
                if (sock->state != SOCKET_STATE_CONNECTED && sock_data->peer_disconnected == 1)
                {
                    // Все данные считаны и сокет закрыт пиром через FIN
                    return 0;
                }
                else  
                {
                    // Мы проснулись, но данные так и не пришли. Вероятно, нас разбудили сигналом
                    return -EINTR;
                }
            }
        }
    }

    acquire_spinlock(&sock_data->rx_queue_lock);

    size_t readable = MIN(len, sock_data->rx_available);

    while (readed < readable)
    {
        size_t remain = readable - readed;

        rcv_buffer = list_head(&sock_data->rx_queue);
        // Сколько осталось в пакете
        size_t available_in_bucket = rcv_buffer->size - rcv_buffer->offset;
        // Сколько можно считать из пакета в буфер
        size_t readable_from_bucket = MIN(available_in_bucket, remain);
        // Скопировать в буфер приёма
        memcpy(buf + readed, rcv_buffer->data + rcv_buffer->offset, readable_from_bucket);

        // Передвинуть смещения
        rcv_buffer->offset += readable_from_bucket;
        readed += readable_from_bucket;
        sock_data->rx_available -= readable_from_bucket;

        if (rcv_buffer->size == rcv_buffer->offset)
        {
            // Весь пакет прочитан - можно его сносить
            list_remove(&sock_data->rx_queue, rcv_buffer);
            kfree(rcv_buffer);
        }
    }

    release_spinlock(&sock_data->rx_queue_lock);

    return readed;
}

int sock_local_sendto_dgram(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    int rc = 0;
    struct sockaddr_un* addr_un = (struct sockaddr_un*) to;
    struct socket* peer_sock = NULL;
    struct file* peer_file = NULL;

    // Указан ли адрес назначения?
    if (to == NULL)
    {
        // Подключен ли сокет?
        if (sock->state == SOCKET_STATE_CONNECTED)
        {
            // TODO: Если да, то отправляем в пира
            return -ENOTSUP;
        }
        else
        {
            // Если не подключен и адресат не указан, то ошибка
            return -EDESTADDRREQ;
        }
    }
    else
    {
        // адрес указан
        // Проверим, что размер адреса минимум sa_family_t
        if (tolen < sizeof(sa_family_t)) {
            return -EINVAL;
        }

        // Проверим значение sa_family
        if (to->sa_family != AF_LOCAL) {
            return -EINVAL;
        }

        // Сколько осталось под адрес сокета
        int namelen = tolen - sizeof(sa_family_t);
        if (namelen < 1) {
            return -EINVAL;
        }
        
        // ищем файл сокета
        peer_file = file_open(NULL, addr_un->sun_path, 0, 0);
        if (peer_file == NULL)
        {
            return -ECONNREFUSED;
        }

        // Проверяем, что это сокет
        if ((peer_file->inode->mode & INODE_FLAG_SOCKET) == 0) {
            // Мы пытаемся открыть файл, который не является сокетом
            rc = -ECONNREFUSED;
            goto exit;
        }

        peer_sock = (struct socket*) peer_file->inode->private_data;
    }

    // Проверим, получилось ли найти объект сокета
    if (peer_sock == NULL)
    {
        rc = -ECONNREFUSED;
        goto exit;
    }

    // Проверим, что это сокет того же типа
    if (peer_sock->domain != AF_LOCAL || peer_sock->type != SOCK_DGRAM)
    {
        return -EPROTOTYPE;
    }

    struct local_socket* peer_data = (struct local_socket*) peer_sock->data;
    if (peer_data == NULL)
    {
        rc = -ECONNREFUSED;
        goto exit;
    }

    // Выделим память сразу и под заголовок и под данные
    unsigned char* bucket = kmalloc(sizeof(struct local_sock_bucket) + len);
    // получим основные указатели
    struct local_sock_bucket* header = (struct local_sock_bucket*) bucket;
    unsigned char* data = (header + 1);
    // запись данных
    memcpy(data, msg, len);
    header->size = len;
    header->offset = 0;

    // добавление пакета в очередь
    acquire_spinlock(&peer_data->rx_queue_lock);
    list_add(&peer_data->rx_queue, header);
    peer_data->rx_available += len;
    release_spinlock(&peer_data->rx_queue_lock);

    // Разбудить одного ожидающего приема
    scheduler_wake(&peer_data->rx_blk, 1);

    // Будим наблюдающих за сокетом пира
    poll_wakeall(&peer_data->poll_wq);

exit:
    if (peer_file != NULL)
        file_close(peer_file);
    return rc;
}

ssize_t sock_local_recvfrom_dgram(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    ssize_t readed = 0;
    struct local_socket* sock_data = (struct local_socket*) sock->data;

    return -ENOTSUP;
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
        default:
            return -ERROR_INVALID_VALUE;
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