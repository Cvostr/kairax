#include "tcp.h"
#include "ipv4.h"
#include "kairax/in.h"
#include "kairax/errors.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "string.h"
#include "drivers/char/random.h"
#include "net/route.h"
#include "kairax/kstdlib.h"
#include "stdio.h"

#define TCP_MAX_PORT        65535
#define TCP_PORTS           65536

#define TCP_DYNAMIC_PORT    49152

#define TCP_HEADER_LEN      (sizeof(struct tcp_packet))
#define TCP_HEADER_SZ_VAL   ((TCP_HEADER_LEN / 4) << 12)

//#define TCP_LOG_PACKET_HANDLER
//#define TCP_LOG_SOCK_CLOSE
//#define TCP_LOG_SOCK_SEND
//#define TCP_LOG_SOCK_RECV
//#define TCP_LOG_NO_SOCK_PORT
#define TCP_LOG_PORT_EXHAUST
//#define TCP_LOG_ACCEPTED_CLIENT

struct ip4_protocol ip4_tcp_protocol = {
    .handler = tcp_ip4_handle
};

struct socket_prot_ops ipv4_stream_ops = {
    .create = sock_tcp4_create,
    .connect = sock_tcp4_connect,
    .accept = sock_tcp4_accept,
    .bind = sock_tcp4_bind,
    .listen = sock_tcp4_listen,
    .recvfrom = sock_tcp4_recvfrom,
    .sendto = sock_tcp4_sendto,
    .close = sock_tcp4_close,
    .setsockopt = sock_tcp4_setsockopt
};

// Сокеты по портам в порядке байтов хоста
struct socket* tcp4_bindings[TCP_PORTS];

int tcp_ip4_handle(struct net_buffer* nbuffer)
{
    struct tcp_packet* tcp_packet = (struct tcp_packet*) nbuffer->transp_header;
    
    // Расчитаем размер сегмента
    uint32_t tcp_header_size = (ntohs(tcp_packet->hlen_flags) & 0xF000) >> 12;
    tcp_header_size *= 4;   // значение приходит в размерности DWORD
    // Сдвинем курсор в буфере
    net_buffer_shift(nbuffer, tcp_header_size);

    // Вычислим указатель на данные и их размер
    nbuffer->payload = nbuffer->transp_header + tcp_header_size;
    nbuffer->payload_size = nbuffer->netw_packet_size - (tcp_header_size);

    // Проверка контрольной суммы
    struct ip4_packet* ip4p = nbuffer->netw_header;
    uint16_t orig_checksum = tcp_packet->checksum;
    tcp_packet->checksum = 0;
    struct tcp_checksum_proto checksum_struct;
    memset(&checksum_struct, 0, sizeof(struct tcp_checksum_proto));
    checksum_struct.dest = ip4p->dst_ip;
    checksum_struct.src = ip4p->src_ip;
    checksum_struct.prot = IPV4_PROTOCOL_TCP;
    checksum_struct.len = htons(nbuffer->netw_packet_size);

    uint16_t calculated = htons(tcp_ip4_calc_checksum(&checksum_struct, tcp_packet, tcp_header_size, nbuffer->payload, nbuffer->payload_size));
    if (calculated != orig_checksum) {
        printk("TCP: Checksum INCORRECT: orig: %i, calc: %i\n", orig_checksum, calculated);
        return -1; // ???  
    }

    // Вычислим флаги и порты
    uint16_t flags = ntohs(tcp_packet->hlen_flags);
    flags &= 0b111111111;
    uint16_t src_port = ntohs(tcp_packet->src_port);
    uint16_t dst_port = ntohs(tcp_packet->dst_port);

#ifdef TCP_LOG_PACKET_HANDLER
    printk("Src port: %i, dest port: %i, fl: %i, header_sz: %i, pl_size: %i\n", src_port, dst_port, flags, tcp_header_size, nbuffer->payload_size);
#endif

    struct socket* sock = tcp4_bindings[dst_port];
    if (sock == NULL)
    {
#ifdef TCP_LOG_NO_SOCK_PORT
        printk("No socket bound to TCP port %i\n", dst_port);
#endif
        // ??? - реализовать
        // Пока не отправляем RST потому что сокет выпадет из списка раньше и уйдет RST на попытку FIN
        //tcp_ip4_err_rst(tcp_packet, ip4p);
        return -1;
    }
    int is_listener = sock->state == SOCKET_STATE_LISTEN;
    int is_syn = (flags & TCP_FLAG_SYNACK) == TCP_FLAG_SYN;

    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    if (is_listener == 1 && !is_syn)
    {
        // Пришли данные по порту сокета-слушателя, при этом соединение уже было установлено
        // Значит надо найти клиентский сокет внутри этого слушателя
        sock = tcp_ip4_listener_get(sock_data, ip4p->src_ip, tcp_packet->src_port);

        if (sock == NULL)
        {
            // ??? - реализовать
            return -1;
        }

        sock_data = (struct tcp4_socket_data*) sock->data;
    }

    if (is_syn) 
    {    
        // Пришел SYN пакет - это первая попытка подключения к серверу
        // Добавляем пакет в backlog

        if (is_listener == 0) {
            // ? CONNREFUSED?
            tcp_ip4_err_rst(tcp_packet, ip4p);
            return 1;
        }

        acquire_spinlock(&sock_data->backlog_lock);
        // запрос соединения
        if (list_size(&sock_data->backlog) < sock_data->backlog_sz) 
        {
            // Добавляем запрос к очереди подключений
            net_buffer_acquire(nbuffer);
            list_add(&sock_data->backlog, nbuffer);

            // Разбудить ожидающих подключений
            scheduler_wake(&sock_data->backlog_blk, 1);
        } else {
            // Очередь заполнена - отправляем RST
            tcp_ip4_err_rst(tcp_packet, ip4p);
            return -1;            
        }

        release_spinlock(&sock_data->backlog_lock);

    } else if ((flags & TCP_FLAG_SYNACK) == TCP_FLAG_SYNACK) 
    {
        // ACK - следующий номер для SN пришедшего пакета
        sock_data->ack = htonl(tcp_packet->sn) + 1;
        // SN - ???
        sock_data->sn += 1;

        tcp_ip4_ack(sock_data);

        // Добавить пакет в очередь приема сокета и разбудить один из потоков
        tcp_ip4_put_to_rx_queue(sock_data, nbuffer);
        
    } else if ((flags & TCP_FLAG_PSH) == TCP_FLAG_PSH) 
    {
        // Пришли данные
        // ACK - прибавим длину нагрузки
        sock_data->ack = htonl(tcp_packet->sn) + nbuffer->payload_size;

        // Ответить ACK
        tcp_ip4_ack(sock_data);

        // Добавить пакет в очередь приема сокета и разбудить один из потоков
        tcp_ip4_put_to_rx_queue(sock_data, nbuffer);
    } else if ((flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)
    {
        sock_data->ack += 1;

        // закрыть соединение
        sock->state = SOCKET_STATE_UNCONNECTED;

        // Разбудить всех ожидающих приема
        // Чтобы они могли сразу выйти с ошибкой
        scheduler_wake(&sock_data->rx_blk, INT_MAX);

        // Ответить ACK
        tcp_ip4_ack(sock_data);
    } else if ((flags & TCP_FLAG_RST) == TCP_FLAG_RST) {

        // закрыть соединение
        sock->state = SOCKET_STATE_UNCONNECTED;

        // Помечаем флаг RST
        sock_data->is_rst = 1;

        // Очищаем очередь приема
        tcp_ip4_sock_drop_recv_buffer(sock_data);

        // Разбудить всех ожидающих приема
        // Чтобы они могли сразу выйти с ошибкой
        scheduler_wake(&sock_data->rx_blk, INT_MAX);

        //printk("RST \n");
    } else if ((flags & TCP_FLAG_ACK) == TCP_FLAG_ACK) {

        //sock_data->ack = htonl(tcp_packet->sn);
        //printk("ACK \n");
    } 
}

void tcp_ip4_put_to_rx_queue(struct tcp4_socket_data* sock_data, struct net_buffer* nbuffer)
{
    acquire_spinlock(&sock_data->rx_queue_lock);

    // Добавить буфер в очередь приема с увеличением количества ссылок
    net_buffer_acquire(nbuffer);
    list_add(&sock_data->rx_queue, nbuffer);
    
    release_spinlock(&sock_data->rx_queue_lock);

    // Разбудить ожидающих приема
    scheduler_wake(&sock_data->rx_blk, 1);
}

void tcp_ip4_sock_drop_recv_buffer(struct tcp4_socket_data* sock)
{
    acquire_spinlock(&sock->rx_queue_lock);
    struct net_buffer* current; 
    while ((current = list_dequeue(&sock->rx_queue)) != NULL)
    {
        net_buffer_free(current);
    }
    release_spinlock(&sock->rx_queue_lock);
}

int tcp_ip4_ack(struct tcp4_socket_data* sock_data)
{
    // Маршрут назначения
    struct route4* route = route4_resolve(sock_data->addr.sin_addr.s_addr);
    if (route == NULL)
    {
        return -ENETUNREACH;
    }

    // Создать буфер запроса SYN
    struct net_buffer* synrq = new_net_buffer_out(1024);
    synrq->netdev = route->interface;

    struct tcp_packet pkt;
    memset(&pkt, 0, TCP_HEADER_LEN);
    pkt.src_port = htons(sock_data->src_port);
    pkt.dst_port = sock_data->addr.sin_port;
    pkt.ack = ntohl(sock_data->ack);
    pkt.sn = ntohl(sock_data->sn);
    pkt.urgent_point = 0;
    pkt.window_size = htons(65535);
    pkt.hlen_flags = htons(TCP_FLAG_ACK | TCP_HEADER_SZ_VAL);
    pkt.checksum = 0;
    
    struct tcp_checksum_proto checksum_struct;
    memset(&checksum_struct, 0, sizeof(struct tcp_checksum_proto));
    checksum_struct.src = synrq->netdev->ipv4_addr;
    checksum_struct.dest = sock_data->addr.sin_addr.s_addr;
    checksum_struct.prot = IPV4_PROTOCOL_TCP;
    checksum_struct.len = htons(TCP_HEADER_LEN);
    pkt.checksum = htons(tcp_ip4_calc_checksum(&checksum_struct, &pkt, TCP_HEADER_LEN, NULL, 0));

    // Добавить заголовок TCP к буферу запроса
    net_buffer_add_front(synrq, &pkt, TCP_HEADER_LEN);

    // Попытка отправить запрос на подключение
    int rc = ip4_send(synrq, route, checksum_struct.dest, IPV4_PROTOCOL_TCP);
    net_buffer_free(synrq);

    return rc;
}

int tcp_ip4_fin(struct tcp4_socket_data* sock_data)
{
    struct route4* route = route4_resolve(sock_data->addr.sin_addr.s_addr);
    if (route == NULL)
    {
        return -ENETUNREACH;
    }

    // Создать буфер запроса FIN
    struct net_buffer* synrq = new_net_buffer_out(1024);
    synrq->netdev = route->interface;

    struct tcp_packet pkt;
    memset(&pkt, 0, TCP_HEADER_LEN);
    pkt.src_port = htons(sock_data->src_port);
    pkt.dst_port = sock_data->addr.sin_port;
    pkt.ack = ntohl(sock_data->ack);
    pkt.sn = ntohl(sock_data->sn);
    pkt.urgent_point = 0;
    pkt.window_size = htons(65535);
    pkt.hlen_flags = htons(TCP_FLAG_FIN | TCP_FLAG_ACK | TCP_HEADER_SZ_VAL);
    pkt.checksum = 0;
    
    struct tcp_checksum_proto checksum_struct;
    memset(&checksum_struct, 0, sizeof(struct tcp_checksum_proto));
    checksum_struct.src = synrq->netdev->ipv4_addr;
    checksum_struct.dest = sock_data->addr.sin_addr.s_addr;
    checksum_struct.prot = IPV4_PROTOCOL_TCP;
    checksum_struct.len = htons(TCP_HEADER_LEN);
    pkt.checksum = htons(tcp_ip4_calc_checksum(&checksum_struct, &pkt, TCP_HEADER_LEN, NULL, 0));

    // Добавить заголовок TCP к буферу запроса
    net_buffer_add_front(synrq, &pkt, TCP_HEADER_LEN);

    // Попытка отправить запрос на завершение
    int rc = ip4_send(synrq, route, checksum_struct.dest, IPV4_PROTOCOL_TCP);
    net_buffer_free(synrq);

    return rc;
}

int tcp_ip4_err_rst(struct tcp_packet* tcp_packet, struct ip4_packet* ip4p)
{
    struct route4* route = route4_resolve(ip4p->src_ip);
    if (route == NULL)
    {
        return -ENETUNREACH;
    }

    // Создать буфер запроса FIN
    struct net_buffer* synrq = new_net_buffer_out(1024);
    synrq->netdev = route->interface;

    struct tcp_packet pkt;
    memset(&pkt, 0, TCP_HEADER_LEN);
    pkt.src_port = tcp_packet->dst_port;
    pkt.dst_port = tcp_packet->src_port;
    pkt.ack = tcp_packet->sn;
    pkt.sn = 0;
    pkt.urgent_point = 0;
    pkt.window_size = htons(65535);
    pkt.hlen_flags = htons(TCP_FLAG_RST | TCP_FLAG_ACK | TCP_HEADER_SZ_VAL);
    pkt.checksum = 0;
    
    struct tcp_checksum_proto checksum_struct;
    memset(&checksum_struct, 0, sizeof(struct tcp_checksum_proto));
    checksum_struct.src = synrq->netdev->ipv4_addr;
    checksum_struct.dest = ip4p->src_ip;
    checksum_struct.prot = IPV4_PROTOCOL_TCP;
    checksum_struct.len = htons(TCP_HEADER_LEN);
    pkt.checksum = htons(tcp_ip4_calc_checksum(&checksum_struct, &pkt, TCP_HEADER_LEN, NULL, 0));

    // Добавить заголовок TCP к буферу запроса
    net_buffer_add_front(synrq, &pkt, TCP_HEADER_LEN);

    // Попытка отправить запрос на завершение
    int rc = ip4_send(synrq, route, checksum_struct.dest, IPV4_PROTOCOL_TCP);
    net_buffer_free(synrq);

    return rc;
}

int tcp_ip4_alloc_dynamic_port(struct socket* sock)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    for (int port = TCP_DYNAMIC_PORT; port < TCP_PORTS; port ++)
    {
        if (tcp4_bindings[port] == NULL)
        {
            sock_data->src_port = port;
            tcp4_bindings[port] = sock;
            return 1;
        }
    }

    return 0;
}

int sock_tcp4_create (struct socket* sock)
{
    sock->data = kmalloc(sizeof(struct tcp4_socket_data));
    if (sock->data == NULL)
    {
        return -ENOMEM;
    }
    
    memset(sock->data, 0, sizeof(struct tcp4_socket_data));
    return 0;
}

int	sock_tcp4_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len)
{
    int rc = 0;
    if (saddr->sa_family != AF_INET || sockaddr_len != sizeof(struct sockaddr_in))
    {
        return -EINVAL;
    }

    struct sockaddr_in* inetaddr = (struct sockaddr_in*) saddr;

    if (sock->state != SOCKET_STATE_UNCONNECTED) {
        return -EISCONN;
    }

    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    if (tcp_ip4_alloc_dynamic_port(sock) == 0)
    {
#ifdef TCP_LOG_PORT_EXHAUST
        printk("TCP: port exhaust!\n");
#endif
        // порты кончились
        return -EADDRNOTAVAIL;
    }

    sock_data->bound_port = sock_data->src_port; 

    memcpy(&sock_data->addr, saddr, sockaddr_len);
    sock->state = SOCKET_STATE_CONNECTING;

    // Маршрут назначения
    struct route4* route = route4_resolve(inetaddr->sin_addr.s_addr);
    if (route == NULL)
    {
        return -ENETUNREACH;
    }

    // Создать буфер запроса SYN
    struct net_buffer* synrq = new_net_buffer_out(1024);
    synrq->netdev = route->interface;

    // Генерируем начальный SN
    sock_data->sn = krand();
    sock_data->ack = 0;

    struct tcp_packet pkt;
    memset(&pkt, 0, TCP_HEADER_LEN);
    pkt.src_port = htons(sock_data->src_port);
    pkt.dst_port = inetaddr->sin_port;
    pkt.ack = sock_data->ack;
    pkt.sn = ntohl(sock_data->sn);
    pkt.urgent_point = 0;
    pkt.window_size = htons(65535);
    pkt.hlen_flags = htons(TCP_FLAG_SYN | TCP_HEADER_SZ_VAL);
    pkt.checksum = 0;

    struct tcp_checksum_proto checksum_struct;
    memset(&checksum_struct, 0, sizeof(struct tcp_checksum_proto));
    checksum_struct.src = synrq->netdev->ipv4_addr;
    checksum_struct.dest = inetaddr->sin_addr.s_addr;
    checksum_struct.prot = IPV4_PROTOCOL_TCP;
    checksum_struct.len = htons(TCP_HEADER_LEN);
    pkt.checksum = htons(tcp_ip4_calc_checksum(&checksum_struct, &pkt, TCP_HEADER_LEN, NULL, 0));

    // Добавить заголовок TCP к буферу запроса
    net_buffer_add_front(synrq, &pkt, TCP_HEADER_LEN);

    // Попытка отправить запрос на подключение
    rc = ip4_send(synrq, route, checksum_struct.dest, IPV4_PROTOCOL_TCP);
    net_buffer_free(synrq);

    if (rc < 0)
        return rc;

    // Ожидание SYN | ACK
    acquire_spinlock(&sock_data->rx_queue_lock);

    if (sock_data->rx_queue.head == NULL) 
    {
        release_spinlock(&sock_data->rx_queue_lock);

        scheduler_sleep_on(&sock_data->rx_blk);

        if (sock_data->is_rst) 
        {
            return -ECONNREFUSED;
        }

        if (sock_data->rx_queue.head == NULL)
        {
            // Мы проснулись, но данные так и не пришли. Вероятно, нас разбудили сигналом
            return -EINTR;
        }

        acquire_spinlock(&sock_data->rx_queue_lock);
    }

    struct net_buffer* synack = list_dequeue(&sock_data->rx_queue);
    //struct tcp_packet* tcpp = (struct tcp_packet*) synack->transp_header;
    //struct ip4_packet* ip4p = (struct ip4_packet*) synack->netw_header;

    release_spinlock(&sock_data->rx_queue_lock);

    //uint16_t flags = ntohs(tcpp->hlen_flags);
    //flags &= 0b111111111;

    net_buffer_free(synack);

    sock->state = SOCKET_STATE_CONNECTED;

    return rc;   
}

void tcp4_fill_sockaddr_in(struct sockaddr_in* dst, struct tcp_packet* tcpp, struct ip4_packet* ip4p)
{
    dst->sin_family = AF_INET;
    dst->sin_addr.s_addr = ip4p->src_ip;
    dst->sin_port = tcpp->src_port;
}

void tcp_ip4_listener_add(struct tcp4_socket_data* listener, struct socket* client)
{
    acquire_spinlock(&listener->children_lock);
    list_add(&listener->children, client);
    release_spinlock(&listener->children_lock);
}

void tcp_ip4_listener_remove(struct tcp4_socket_data* listener, struct socket* client)
{
    acquire_spinlock(&listener->children_lock);
    list_remove(&listener->children, client);
    release_spinlock(&listener->children_lock);
}

struct socket* tcp_ip4_listener_get(struct tcp4_socket_data* listener, uint32_t addr, uint16_t port)
{
    acquire_spinlock(&listener->children_lock);

    struct socket* result = NULL;
    struct list_node* current = listener->children.head;
    while (current != NULL)
    {
        struct socket* sock = current->element;
        struct tcp4_socket_data* sockdata = (struct tcp4_socket_data*) sock->data; 

        if (sock != NULL && sockdata->addr.sin_port == port && sockdata->addr.sin_addr.s_addr == addr)
        {
            result = sock;
            break;
        }

        current = current->next;
    }

    release_spinlock(&listener->children_lock);

    return result;
}

int	sock_tcp4_accept(struct socket *sock, struct socket **newsock, struct sockaddr *addr)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    // Вытаскиваем из начала списка
    struct net_buffer* syn_req_buffer = list_dequeue(&sock_data->backlog);

    if (syn_req_buffer == NULL)
    {
        // Ожидание подключений
        scheduler_sleep_on(&sock_data->backlog_blk);

        // Пробуем вытащить пакет
        acquire_spinlock(&sock_data->rx_queue_lock);
        syn_req_buffer = list_dequeue(&sock_data->backlog);
        release_spinlock(&sock_data->rx_queue_lock);

        if (syn_req_buffer == NULL)
        {
            // Мы проснулись, но пакетов нет - выходим с EINTR
            return -EINTR;
        }
    }

    // Извлекаем заголовки
    struct tcp_packet* tcpp = (struct tcp_packet*) syn_req_buffer->transp_header;
    struct ip4_packet* ip4p = (struct ip4_packet*) syn_req_buffer->netw_header;

    // Создание сокета клиента
    struct socket* client_sock = new_socket();
    sock_tcp4_create(client_sock);
    client_sock->ops = &ipv4_stream_ops;

    struct tcp4_socket_data* client_sockdata = (struct tcp4_socket_data*) client_sock->data;

    // Биндинг сокета клиента
    tcp4_fill_sockaddr_in(&client_sockdata->addr, tcpp, ip4p);
    // Общаемся с клиентом от имени порта сервера
    client_sockdata->src_port = sock_data->bound_port;
    // Биндинг сокета клиента по порту источника
    client_sockdata->bound_port = htons(client_sockdata->addr.sin_port);
    
    // Указатель на listener
    client_sockdata->listener = sock_data;
    // Добавить в список клиентских сокетов
    tcp_ip4_listener_add(sock_data, client_sock);

#ifdef TCP_LOG_ACCEPTED_CLIENT
    union ip4uni src;
	src.val = client_sockdata->addr.sin_addr.s_addr; 
	printk("Accepted client: IP4 : %i.%i.%i.%i, port: %i,\n", src.array[0], src.array[1], src.array[2], src.array[3], client_sockdata->addr.sin_port);
#endif

    // Маршрут назначения
    struct route4* route = route4_resolve(ip4p->src_ip);
    if (route == NULL)
    {
        return -ENETUNREACH;
    }

    // Готовим ответ SYN | ACK
    struct net_buffer* resp = new_net_buffer_out(4096);
    resp->netdev = route->interface;

    int flags = (TCP_FLAG_SYN | TCP_FLAG_ACK);
    
    // Генерируем начальный SN
    client_sockdata->sn = krand();
    client_sockdata->ack = ntohl(tcpp->sn);

    int header_len = (sizeof(struct tcp_packet));

    struct tcp_packet pkt;
    memset(&pkt, 0, sizeof(struct tcp_packet));
    pkt.src_port = htons(client_sockdata->src_port);
    pkt.dst_port = client_sockdata->addr.sin_port;
    pkt.ack = ntohl(client_sockdata->ack + 1);
    pkt.sn = ntohl(client_sockdata->sn);
    pkt.urgent_point = 0;
    pkt.window_size = htons(65535);
    pkt.hlen_flags = htons(flags | TCP_HEADER_SZ_VAL);
    pkt.checksum = 0;

    struct tcp_checksum_proto checksum_struct;
    memset(&checksum_struct, 0, sizeof(struct tcp_checksum_proto));
    checksum_struct.dest = ip4p->src_ip;
    checksum_struct.src = ip4p->dst_ip;
    checksum_struct.prot = IPV4_PROTOCOL_TCP;
    checksum_struct.len = htons(sizeof(struct tcp_packet));
    pkt.checksum = htons(tcp_ip4_calc_checksum(&checksum_struct, &pkt, header_len, NULL, 0));

    // Добавить заголовок TCP к буферу  ответа
    net_buffer_add_front(resp, &pkt, sizeof(struct tcp_packet));
    
    // Освободить буфер с запросом SYN
    net_buffer_free(syn_req_buffer);

    // Попытка отправить ответ
    ip4_send(resp, route, checksum_struct.dest, IPV4_PROTOCOL_TCP);
    net_buffer_free(resp);

    // TODO: завершить handshake - ожидание ACK

    client_sock->state = SOCKET_STATE_CONNECTED;
    *newsock = client_sock;

    return 0;
}

int sock_tcp4_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen)
{
    if (addr->sa_family != AF_INET || addrlen != sizeof(struct sockaddr_in))
    {
        return -EINVAL;
    }

    struct sockaddr_in* inetaddr = (struct sockaddr_in*) addr;
    uint16_t port = ntohs(inetaddr->sin_port);

    if (tcp4_bindings[port] != NULL)
    {
        // TODO: учитывать разные IP адреса и сетевые карты
        return -EADDRINUSE;
    }

    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    sock_data->addr.sin_port = inetaddr->sin_port;

    // Bind port
    sock_data->bound_port = port;
    tcp4_bindings[port] = sock;

    return 0;
}

int sock_tcp4_listen(struct socket* sock, int backlog)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    sock_data->backlog_sz = backlog;

    // Принимающий подключения
    sock->state = SOCKET_STATE_LISTEN;

    // На всякий случай
    sock_data->src_port = sock_data->bound_port;

    return 0;
}

ssize_t sock_tcp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
#ifdef TCP_LOG_SOCK_RECV
    printk("tcp: recv()\n");
#endif
    if (sock->state != SOCKET_STATE_CONNECTED) {
        return -ENOTCONN;
    }

    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    struct net_buffer* rcv_buffer = list_head(&sock_data->rx_queue);

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

            // Пробуем вытащить пакет
            acquire_spinlock(&sock_data->rx_queue_lock);
            rcv_buffer = list_head(&sock_data->rx_queue);
            release_spinlock(&sock_data->rx_queue_lock);

            // Пришел RST
            if (sock_data->is_rst && rcv_buffer == NULL) 
            {
                return -ECONNRESET;
            }

            if (rcv_buffer == NULL)
            {
                if (sock->state != SOCKET_STATE_CONNECTED)
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

    size_t readable = MIN(len, rcv_buffer->payload_size);

    memcpy(buf, rcv_buffer->cursor, readable);

    net_buffer_shift(rcv_buffer, readable);
    rcv_buffer->payload_size -= readable;

    if (rcv_buffer->payload_size == 0)
    {
        acquire_spinlock(&sock_data->rx_queue_lock);
        list_remove(&sock_data->rx_queue, rcv_buffer);
        release_spinlock(&sock_data->rx_queue_lock);

        net_buffer_free(rcv_buffer);
    }

    return readable;
}

int sock_tcp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
#ifdef TCP_LOG_SOCK_SEND
    printk("tcp: send()\n");
#endif

    if (sock->state != SOCKET_STATE_CONNECTED) {
        return -ENOTCONN;
    }

    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    // Маршрут назначения
    in_addr_t dst_addr = sock_data->addr.sin_addr.s_addr;
    struct route4* route = route4_resolve(dst_addr);
    if (route == NULL)
    {
        return -ENETUNREACH;
    }

    // Создать буфер запроса SYN
    struct net_buffer* synrq = new_net_buffer_out(1024);
    synrq->netdev = route->interface;

    // Добавить в буфер нагрузку
    net_buffer_add_front(synrq, msg, len);

    struct tcp_packet pkt;
    memset(&pkt, 0, TCP_HEADER_LEN);
    pkt.src_port = htons(sock_data->src_port);
    pkt.dst_port = sock_data->addr.sin_port;
    pkt.ack = ntohl(sock_data->ack);
    pkt.sn = ntohl(sock_data->sn);
    pkt.urgent_point = 0;
    pkt.window_size = htons(65535);
    pkt.hlen_flags = htons(TCP_FLAG_PSH | TCP_FLAG_ACK | TCP_HEADER_SZ_VAL);
    pkt.checksum = 0;

    struct tcp_checksum_proto checksum_struct;
    memset(&checksum_struct, 0, sizeof(struct tcp_checksum_proto));
    checksum_struct.src = synrq->netdev->ipv4_addr;
    checksum_struct.dest = dst_addr;
    checksum_struct.prot = IPV4_PROTOCOL_TCP;
    checksum_struct.len = htons(TCP_HEADER_LEN + len);
    pkt.checksum = htons(tcp_ip4_calc_checksum(&checksum_struct, &pkt, TCP_HEADER_LEN, msg, len));

    // Добавить заголовок TCP к буферу запроса
    net_buffer_add_front(synrq, &pkt, TCP_HEADER_LEN);

    sock_data->sn += len;

    // Попытка отправить запрос на подключение
    int rc = ip4_send(synrq, route, checksum_struct.dest, IPV4_PROTOCOL_TCP);
    net_buffer_free(synrq);

    return rc;
}

int sock_tcp4_close(struct socket* sock)
{
#ifdef TCP_LOG_SOCK_CLOSE
    printk("tcp: close()\n");
#endif
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    if (sock->state != SOCKET_STATE_LISTEN)
    {
        int rc = tcp_ip4_fin(sock_data);
        if (rc != 0)
        {
            printk("tcp: close() error: %i", rc);
        }
    }

    sock->state == SOCKET_STATE_UNCONNECTED;

    if (sock_data) 
    {
        // Если закрываемый сокет - дочерний сокет от сервера
        // То надо удалить его из этого серверного сокета 
        if (sock_data->listener != NULL) 
        {
            tcp_ip4_listener_remove(sock_data->listener, sock);
            sock_data->listener = NULL;
        }

        // Разбудить всех ожидающих приема
        // Чтобы они могли дочитать буфер или сразу выйти с ошибкой
        scheduler_wake(&sock_data->rx_blk, INT_MAX);

        // Освободить порт
        if (tcp4_bindings[sock_data->bound_port] == sock)
        {
            tcp4_bindings[sock_data->bound_port] = NULL;
        }

        // TODO: должно происходить только при освобождении inode
        // Очищаем очередь приема
        tcp_ip4_sock_drop_recv_buffer(sock_data);

        kfree(sock_data);
    }

    return 0;
}

int sock_tcp4_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen)
{
    return 0;
}

void tcp_ip4_init()
{
    ip4_register_protocol(&ip4_tcp_protocol, IPV4_PROTOCOL_TCP);
}

uint16_t tcp_ip4_calc_checksum(struct tcp_checksum_proto* prot, struct tcp_packet* header, size_t header_size, const unsigned char* payload, size_t payload_size)
{
    uint32_t sum = 0;
    uint16_t* data = (uint16_t*) prot;

    uint8_t prot_header_sz = sizeof(struct tcp_checksum_proto) / sizeof(uint16_t);
    uint8_t header_sz = header_size / sizeof(uint16_t);
    uint32_t i;

    for (i = 0; i < prot_header_sz; ++i) {
		sum += ntohs(data[i]);
		if (sum > 0xFFFF) {
			sum = (sum >> 16) + (sum & 0xFFFF);
		}
	}

	data = (uint16_t*) header;
	for (i = 0; i < header_sz; ++i) {
		sum += ntohs(data[i]);
		if (sum > 0xFFFF) {
			sum = (sum >> 16) + (sum & 0xFFFF);
		}
	}

	uint16_t d_words = payload_size / sizeof(uint16_t);

	data = (uint16_t*) payload;
	for (i = 0; i < d_words; ++i) {
		sum += ntohs(data[i]);
		if (sum > 0xFFFF) {
			sum = (sum >> 16) + (sum & 0xFFFF);
		}
	}

	if (d_words * 2 != payload_size) {
		uint8_t tmp[2];
		tmp[0] = payload[d_words * sizeof(uint16_t)];
		tmp[1] = 0;

		uint16_t * f = (uint16_t *)tmp;

		sum += ntohs(f[0]);
		if (sum > 0xFFFF) {
			sum = (sum >> 16) + (sum & 0xFFFF);
		}
	}

	return (uint16_t) (~(sum & 0xFFFF));
}