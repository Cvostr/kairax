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
#include "kairax/siphash.h"
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

struct siphash_key syncookie_key;

struct socket_prot_ops ipv4_stream_ops = {
    .create = sock_tcp4_create,
    .connect = sock_tcp4_connect,
    .accept = sock_tcp4_accept,
    .bind = sock_tcp4_bind,
    .listen = sock_tcp4_listen,
    .recvfrom = sock_tcp4_recvfrom,
    .sendto = sock_tcp4_sendto,
    .close = sock_tcp4_close,
    .shutdown = sock_tcp4_shutdown,
    .poll = sock_tcp4_poll,
    .setsockopt = sock_tcp4_setsockopt,
    .getpeername = sock_tcp4_getpeername
};

// Сокеты по портам в порядке байтов хоста
struct socket* tcp4_bindings[TCP_PORTS];

void tcpdump()
{
    for (int i = 0; i < TCP_PORTS; i ++)
    {
        struct socket* sck = tcp4_bindings[i];
        if (sck != NULL)
        {
            struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sck->data;
            printk("Socket port %i, state %i\n", sock_data->bound_port, sck->state);

            if (sock_data->is_listener)
            {
                struct list_node* current = sock_data->children.head;
                while (current != NULL)
                {
                    struct socket* sock = current->element;
                    struct tcp4_socket_data* sockdata = (struct tcp4_socket_data*) sock->data; 

                    union ip4uni src;
	                src.val = sockdata->addr.sin_addr.s_addr; 

                    printk("  Socket - bound port %i, src port %i, state %i peer %i.%i.%i.%i\n", sockdata->bound_port, sock_data->src_port, sock->state, 
                    src.array[0], src.array[1], src.array[2], src.array[3]);

                    current = current->next;
                }
            }
        }
    }
}

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
        tcp_ip4_err_rst(tcp_packet, ip4p);
        return -1;
    }

    int rc = 0;
    acquire_socket(sock);

    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    // Является ли сокет - слушающим?
    int is_listener = sock_data->is_listener;
    // Является ли сокет активно слушающим? (после close перестает быть таковым)
    int is_active_listener = sock->state == SOCKET_STATE_LISTEN;
    // Сообщение - первая попытка подключения?
    int is_syn = (flags & TCP_FLAG_SYNACK) == TCP_FLAG_SYN;
    int is_ack = (flags & TCP_FLAG_ACK) == TCP_FLAG_ACK;

    if (is_listener == 1 && !is_syn)
    {
        // Пришли данные по порту сокета-слушателя, при этом соединение уже было установлено
        // Значит надо найти клиентский сокет внутри этого слушателя
        struct socket* child_sock = tcp_ip4_listener_get(sock, ip4p->src_ip, tcp_packet->src_port);

        if (child_sock == NULL)
        {
            // Не нашли сокет, пробуем обработь ожидаемые подключения
            if (((flags & TCP_FLAG_ACK) == TCP_FLAG_ACK) && tcp_ip4_listener_handle_ack(sock, nbuffer) == TRUE)
            {
                rc = 0;
                goto exit;
            }

            // отсутствует сокет
            tcp_ip4_err_rst(tcp_packet, ip4p);
            rc = -1;
            goto exit;
        }


        // Переопределяем sock и sock_data, чтобы дальше работать уже с ними
        free_socket(sock);
        sock = child_sock;
        acquire_socket(sock);
        sock_data = (struct tcp4_socket_data*) sock->data;
    }

    if (is_syn) 
    {    
        // Пришел SYN пакет - это первая попытка подключения к серверу
        if (is_active_listener == 0) {
            // ? CONNREFUSED?
            tcp_ip4_err_rst(tcp_packet, ip4p);
            rc = 1;
            goto exit;
        }

        tcp_ip4_handle_syn(sock, nbuffer);
    } 
    else if ((flags & TCP_FLAG_SYNACK) == TCP_FLAG_SYNACK) 
    {
        // ACK - следующий номер для SN пришедшего пакета
        sock_data->ack = htonl(tcp_packet->sn) + 1;
        // SN - ???
        sock_data->sn += 1;

        tcp_ip4_ack(sock_data);

        // Ставим статус подключен
        sock->state = SOCKET_STATE_CONNECTED;

        // Разбудить ожидающих приема
        scheduler_wake(&sock_data->rx_blk, 1);
        // Будим наблюдающих
        poll_wakeall(&sock_data->rx_poll_wq);
    } 
    else if ((flags & TCP_FLAG_PSH) == TCP_FLAG_PSH) 
    {
        // Пришли данные
        // ACK - прибавим длину нагрузки
        sock_data->ack = htonl(tcp_packet->sn) + nbuffer->payload_size;

        // Ответить ACK
        tcp_ip4_ack(sock_data);

        // Добавить пакет в очередь приема сокета и разбудить один из потоков
        tcp_ip4_put_to_rx_queue(sock_data, nbuffer);
    } 
    else if ((flags & TCP_FLAG_FIN) == TCP_FLAG_FIN)
    {
        tcp_ip4_handle_fin(sock, nbuffer, is_ack);
        rc = 0;
        goto exit;
    }
    else if ((flags & TCP_FLAG_RST) == TCP_FLAG_RST) {

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
    } 
    else if (is_ack) 
    {
        tcp_ip4_handle_ack(sock, nbuffer);
    } 

exit:
    free_socket(sock);
    return rc;
}

void tcp_ip4_destroy(struct socket* sock)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    sock->state = SOCKET_STATE_UNCONNECTED;

    // TODO: должно происходить только при освобождении inode
    // Очищаем очередь приема
    tcp_ip4_sock_drop_recv_buffer(sock_data);

    // Если закрываемый сокет - дочерний сокет от сервера
    // То надо удалить его из этого серверного сокета 
    if (sock_data->listener != NULL) 
    {
        // Удаляем сокет из списка дочерних
        // Если ссылок не осталось, то сокет уничтожится
        tcp_ip4_listener_remove(sock_data->listener, sock);
        return;
    }

    // Освободить порт
    // Если ссылок больше не останется, то сокет будет уничтожен
    tcp_ip4_unbind_sock(sock);
}

void tcp_ip4_handle_ack(struct socket* sock, struct net_buffer* nbuffer)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    
    if (sock->state == SOCKET_STATE_LAST_ACK)
    {
        tcp_ip4_destroy(sock);
        return;
    }

    if (sock->state == SOCKET_STATE_CLOSING)
    {
        tcp_ip4_destroy(sock);
        return;
    }

    if (sock->state == SOCKET_STATE_FIN_WAIT1)
    {
        sock->state = SOCKET_STATE_FIN_WAIT2;
        return;
    }
}

void tcp_ip4_handle_fin(struct socket* sock, struct net_buffer* nbuffer, int has_ack)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    if (sock->state == SOCKET_STATE_CONNECTED)
    {
        // Статус - ожидание закрытия со стороны приложения
        sock->state = SOCKET_STATE_CLOSE_WAIT;

        // Закрыли запись с той стороны, значит мы со своей стороны закрываем чтение
        sock_data->shut_rd = TRUE;

        sock_data->ack += 1;
        // Ответить ACK
        tcp_ip4_ack(sock_data);

        // Разбудить всех ожидающих приема
        // Чтобы они могли сразу выйти с ошибкой
        scheduler_wake(&sock_data->rx_blk, INT_MAX);

        // Будим наблюдающих
        poll_wakeall(&sock_data->rx_poll_wq);
        return;
    }

    if (sock->state == SOCKET_STATE_FIN_WAIT1 && has_ack == FALSE)
    {
        // Статус - закрываемся)))
        sock->state = SOCKET_STATE_CLOSING;

        // отправляем ACK с увеличением ack
        sock_data->ack += 1;
        // Ответить ACK
        tcp_ip4_ack(sock_data);
        return;
    }

    if (sock->state == SOCKET_STATE_FIN_WAIT1 && has_ack == TRUE)
    {
        // отправляем ACK с увеличением ack
        sock_data->ack += 1;
        // Ответить ACK
        tcp_ip4_ack(sock_data);

        tcp_ip4_destroy(sock);
        return;
    }

    if (sock->state == SOCKET_STATE_FIN_WAIT2)
    {
        // отправляем ACK с увеличением ack
        sock_data->ack += 1;
        // Ответить ACK
        tcp_ip4_ack(sock_data);

        tcp_ip4_destroy(sock);
        return;
    }
}

uint32_t tcp_ip4_make_syn_cookie(uint32_t saddr, uint32_t destaddr, uint16_t srcport, uint16_t dstport, uint32_t seq)
{
    // Получаем текущее время
    struct timeval tv;
    arch_get_time_epoch(&tv);

    uint32_t ports = ((uint32_t) srcport) << 16 | dstport; 

    // Одна из компонент cookie. Время в секундах.
    uint32_t t = tv.tv_sec >> 6;

    uint32_t s = siphash_4u32(saddr, destaddr, ports, t, &syncookie_key);

    return s;
}

void tcp_ip4_handle_syn(struct socket* sock, struct net_buffer* nbuffer)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    // Вытащим указатели на заголовки L4 и L3
    struct tcp_packet* tcp_packet = (struct tcp_packet*) nbuffer->transp_header;
    struct ip4_packet* ip4p = nbuffer->netw_header;

#ifdef TCP_LOG_ACCEPTED_CLIENT
    union ip4uni src;
	src.val = ip4p->src_ip; 
	printk("SYN client: IP4 : %i.%i.%i.%i, port: %i, sn %u, ack %u - unf SN %u\n", src.array[0], src.array[1], src.array[2], src.array[3],
        ntohs(tcp_packet->src_port),
        ntohl(tcp_packet->sn),
        ntohl(tcp_packet->ack),
        tcp_packet->sn);
#endif

    // Считаем Cookie
    uint32_t sn = tcp_ip4_make_syn_cookie(ip4p->src_ip, ip4p->dst_ip, tcp_packet->src_port, tcp_packet->dst_port, tcp_packet->sn);
    uint32_t ack = ntohl(tcp_packet->sn) + 1;

    // Отправляем SYNACK
    // Маршрут назначения
    struct route4* route = route4_resolve(ip4p->src_ip);
    if (route == NULL)
    {
        printk("NO ROUTE ON SYNACK\n");
        return;
    }

    // Готовим ответ SYN | ACK
    struct net_buffer* resp = new_net_buffer_out(2048);
    resp->netdev = route->interface;

    int flags = (TCP_FLAG_SYN | TCP_FLAG_ACK);

    struct tcp_packet pkt;
    memset(&pkt, 0, TCP_HEADER_LEN);
    pkt.src_port = tcp_packet->dst_port;
    pkt.dst_port = tcp_packet->src_port;
    pkt.ack = ntohl(ack);
    pkt.sn = ntohl(sn);
    pkt.urgent_point = 0;
    pkt.window_size = htons(65535);
    pkt.hlen_flags = htons(flags | TCP_HEADER_SZ_VAL);
    pkt.checksum = 0;

    struct tcp_checksum_proto checksum_struct;
    memset(&checksum_struct, 0, sizeof(struct tcp_checksum_proto));
    checksum_struct.dest = ip4p->src_ip;
    checksum_struct.src = ip4p->dst_ip;
    checksum_struct.prot = IPV4_PROTOCOL_TCP;
    checksum_struct.len = htons(TCP_HEADER_LEN);
    pkt.checksum = htons(tcp_ip4_calc_checksum(&checksum_struct, &pkt, TCP_HEADER_LEN, NULL, 0));

    // Добавить заголовок TCP к буферу  ответа
    net_buffer_add_front(resp, &pkt, TCP_HEADER_LEN);
    
    // Попытка отправить ответ
    int rc = ip4_send(resp, route, checksum_struct.dest, IPV4_PROTOCOL_TCP);
    if (rc != 0)
        printk("Send rc %i\n", -rc);

    net_buffer_free(resp);
}

int tcp_ip4_listener_handle_ack(struct socket* sock, struct net_buffer* nbuffer)
{
    int handled = FALSE;
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    // Вытащим указатели на заголовки L4 и L3
    struct tcp_packet* tcp_packet = (struct tcp_packet*) nbuffer->transp_header;
    struct ip4_packet* ip4p = nbuffer->netw_header;

    // Получаем старый SN клиента (который использовался на момент создания Cookie)
    uint32_t old_client_sn = htonl(ntohl(tcp_packet->sn) - 1);
    // Получаем старый SN сервера (который должен быть равен новой cookie)
    uint32_t old_server_sn = ntohl(tcp_packet->ack) - 1;
    // Считаем Cookie еще раз
    uint32_t calculated_cookie = tcp_ip4_make_syn_cookie(ip4p->src_ip, ip4p->dst_ip, tcp_packet->src_port, tcp_packet->dst_port, old_client_sn);

    if (calculated_cookie == old_server_sn)
    {
        if (list_size(&sock_data->accept_queue) >= sock_data->backlog_sz)
        {
            printk("ACCEPT QUEUE IS FULL\n");
            //tcp_ip4_err_rst(tcp_packet, ip4p);
            return FALSE;
        }

        // Заполним структуру запроса соединения
        struct tcp_connection_request *request = kmalloc(sizeof(struct tcp_connection_request));
        request->addr = ip4p->src_ip;
        request->port = tcp_packet->src_port;
        // Сразу заполним SN и ACK
        request->sn = old_server_sn;
        request->ack = ntohl(tcp_packet->sn);

        acquire_spinlock(&sock_data->accept_queue_lock);
        // Добавляем запрос к очереди SYN
        list_add(&sock_data->accept_queue, request);

        release_spinlock(&sock_data->accept_queue_lock);

        // Будим ожидающих подключений
        scheduler_wake(&sock_data->backlog_blk, 1);

        // Будим наблюдающих
        poll_wakeall(&sock_data->rx_poll_wq);

        return TRUE;
    }

    return FALSE;
}

void tcp_ip4_put_to_rx_queue(struct tcp4_socket_data* sock_data, struct net_buffer* nbuffer)
{
    if (sock_data->shut_rd == TRUE)
    {
        // после shutdown(SHUT_RD) больше ничего не принимаем
        return;
    }

    acquire_spinlock(&sock_data->rx_queue_lock);

    // Добавить буфер в очередь приема с увеличением количества ссылок
    net_buffer_acquire(nbuffer);
    list_add(&sock_data->rx_queue, nbuffer);
    
    release_spinlock(&sock_data->rx_queue_lock);

    // Разбудить ожидающих приема
    scheduler_wake(&sock_data->rx_blk, 1);
    // Будим наблюдающих
    poll_wakeall(&sock_data->rx_poll_wq);
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
            acquire_socket(sock);
            return 1;
        }
    }

    return 0;
}

void tcp_ip4_unbind_sock(struct socket* sock)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    if (tcp4_bindings[sock_data->bound_port] == sock)
    {
        tcp4_bindings[sock_data->bound_port] = NULL;
        free_socket(sock);
    }
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

    // TODO: если сокет неблокирующийся, выходить на этом моменте с -EINPGROGRESS

    // Проверяем, успели ли мы подключиться
    // Получение SYNACK, отправка ACK, установка статуса будет производиться в фоне (tcp_ip4_handle())
    while (sock->state != SOCKET_STATE_CONNECTED)
    {
        // Засыпаем
        // TODO: добавить таймаут с раундами
        if (scheduler_sleep_on(&sock_data->rx_blk) == WAKE_SIGNAL)
        {
            return -EINTR;
        }

        // Пришел RST
        if (sock_data->is_rst) 
        {
            return -ECONNREFUSED;
        }
    }

    return 0;
}

void tcp4_fill_sockaddr_in(struct sockaddr_in* dst, struct tcp_packet* tcpp, struct ip4_packet* ip4p)
{
    dst->sin_family = AF_INET;
    dst->sin_addr.s_addr = ip4p->src_ip;
    dst->sin_port = tcpp->src_port;
}

void tcp_ip4_listener_add(struct socket* listener, struct socket* client)
{
    struct tcp4_socket_data* listener_data = listener->data;

    acquire_spinlock(&listener_data->children_lock);
    list_add(&listener_data->children, client);
    release_spinlock(&listener_data->children_lock);

    acquire_socket(client);
}

void tcp_ip4_listener_remove(struct socket *sock, struct socket* client)
{
    struct tcp4_socket_data* listener_data = sock->data;

    acquire_spinlock(&listener_data->children_lock);
    list_remove(&listener_data->children, client);
    int remaining = list_size(&listener_data->children);
    release_spinlock(&listener_data->children_lock);

    free_socket(client);

    if (sock->state == SOCKET_STATE_UNCONNECTED && remaining == 0)
    {
        // Сокет закрыт и у него больше не осталось детей
        // Снимаем биндинг сокета по порту и (вероятно) уничтожаем
        tcp_ip4_unbind_sock(sock);
    }
}

struct socket* tcp_ip4_listener_get(struct socket* listener, uint32_t addr, uint16_t port)
{
    struct tcp4_socket_data* listener_data = (struct tcp4_socket_data*) listener->data;
    acquire_spinlock(&listener_data->children_lock);

    struct socket* result = NULL;
    struct list_node* current = listener_data->children.head;
    while (current != NULL)
    {
        struct socket* sock = current->element;
        struct tcp4_socket_data* sockdata = (struct tcp4_socket_data*) sock->data; 

        if (sock != NULL && (sockdata->addr.sin_port == port) && (sockdata->addr.sin_addr.s_addr == addr))
        {
            result = sock;
            break;
        }

        current = current->next;
    }

    release_spinlock(&listener_data->children_lock);

    return result;
}

int	sock_tcp4_accept(struct socket *sock, struct socket **newsock, struct sockaddr *addr)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    struct tcp_connection_request *conn_request = list_dequeue(&sock_data->accept_queue);

    if (conn_request == NULL)
    {
        // Ожидание подключений
        scheduler_sleep_on(&sock_data->backlog_blk);

        // Пробуем вытащить пакет
        acquire_spinlock(&sock_data->accept_queue_lock);
        conn_request = list_dequeue(&sock_data->accept_queue);
        release_spinlock(&sock_data->accept_queue_lock);

        if (conn_request == NULL)
        {
            // Мы проснулись, но пакетов нет - выходим с EINTR
            return -EINTR;
        }
    }
    
    // Создание сокета клиента
    struct socket* client_sock = new_socket();
    sock_tcp4_create(client_sock);
    client_sock->ops = &ipv4_stream_ops;

    struct tcp4_socket_data* client_sockdata = (struct tcp4_socket_data*) client_sock->data;

    // Биндинг сокета клиента - заполняем адрес пира
    client_sockdata->addr.sin_family = AF_INET;
    client_sockdata->addr.sin_addr.s_addr = conn_request->addr;
    client_sockdata->addr.sin_port = conn_request->port;
    // Общаемся с клиентом от имени порта сервера
    client_sockdata->src_port = sock_data->bound_port;
    // Биндинг сокета клиента по порту источника
    client_sockdata->bound_port = htons(client_sockdata->addr.sin_port);
    // Заполняем SN ACK
    client_sockdata->sn = conn_request->sn;
    client_sockdata->ack = conn_request->ack;
    // Указатель на listener
    client_sockdata->listener = sock;
    // Добавить в список клиентских сокетов
    tcp_ip4_listener_add(sock, client_sock);

    client_sock->state = SOCKET_STATE_CONNECTED;
    *newsock = client_sock;
    
    kfree(conn_request);
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
    acquire_socket(sock);

    return 0;
}

int sock_tcp4_listen(struct socket* sock, int backlog)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    sock_data->backlog_sz = backlog;

    // Принимающий подключения
    sock_data->is_listener = TRUE;
    
    // Активно принимающий подключения
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

    while (rcv_buffer == NULL) 
    {
        // неблокирующее чтение
        if ((flags & MSG_DONTWAIT) == MSG_DONTWAIT)
        {
            return -EAGAIN;
        }

        // shutdown(SHUT_RD)
        if (sock_data->shut_rd == TRUE)
        {
            return 0;
        }

        // Пришел RST
        if (sock_data->is_rst) 
        {
            return -ECONNRESET;
        }

        // Ожидание пакета
        if (scheduler_sleep_on(&sock_data->rx_blk) == WAKE_SIGNAL)
        {
            // нас разбудили сигналом
            return -EINTR;
        }

        // Пробуем вытащить пакет
        acquire_spinlock(&sock_data->rx_queue_lock);
        rcv_buffer = list_head(&sock_data->rx_queue);
        release_spinlock(&sock_data->rx_queue_lock);

        if ((sock->state != SOCKET_STATE_CONNECTED) && rcv_buffer == NULL)
        {
            // Все данные считаны и сокет закрыт пиром через FIN
            return 0;
        }
    }

    // Считаем нагрузку из пакета
    size_t readable = net_buffer_read_payload_into(rcv_buffer, buf, len);

    // Остались ли данные в пакете?
    if (net_buffer_get_payload_remain_len(rcv_buffer) == 0)
    {
        // Не осталось - удаляем пакет из очереди
        acquire_spinlock(&sock_data->rx_queue_lock);
        list_remove(&sock_data->rx_queue, rcv_buffer);
        release_spinlock(&sock_data->rx_queue_lock);

        // Уменьшаем ссылки
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

    if (sock_data->shut_wr == TRUE)
    {
        // shutdown(SHUT_WR)
        return -EPIPE;
    }

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

    if (sock_data->is_listener == FALSE)
    {
        // Мы не сокет-слушатель.
        if (sock->state == SOCKET_STATE_CLOSE_WAIT)
        {
            // Сокет был закрыт со стороны пира и в ожидании закрытия приложением
            // Переводим его в ожидание ACK
            sock->state = SOCKET_STATE_LAST_ACK; 
        }
        else if (sock->state == SOCKET_STATE_CONNECTED) 
        {
            // Соединение было, значит закрываем сами
            sock->state = SOCKET_STATE_FIN_WAIT1;
        }
        else if (sock->state == SOCKET_STATE_CONNECTING)
        {
            // Мы еще только подключаемся
            tcp_ip4_unbind_sock(sock);
        }

        // Надо отправить FIN в сторону пира
        // TODO: не отправлять после SHUTDOWN_WR? (sock_data->shut_wr == FALSE)
        int rc = tcp_ip4_fin(sock_data);
        if (rc != 0)
        {
            printk("tcp: close() error: %i", rc);
        }
    }
    else if (sock_data->is_listener == TRUE)
    {
        // Мы слушатель
        // Запрещаем новые подключения, снимая признак активного слушателя
        // И ждем, пока отключатся все дочерние сокеты
        sock->state = SOCKET_STATE_UNCONNECTED;   

        if (list_size(&sock_data->children) == 0)
        {
            // Если детей уже не было, то сразу
            // Снимаем биндинг сокета по порту и (вероятно) уничтожаем
            tcp_ip4_unbind_sock(sock);
        }
    }

    return 0;
}

int sock_tcp4_getpeername(struct socket *sock, struct sockaddr *addr, socklen_t *addrlen)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    // Проверим, что сокет подключен
    if (sock->state != SOCKET_STATE_CONNECTED)
    {
        return -ENOTCONN;
    }
    
    // Копируем, сколько можем
    size_t to_copy_sz = *addrlen < sizeof(struct sockaddr_in) ? *addrlen : sizeof(struct sockaddr_in);
    memcpy(addr, &sock_data->addr, to_copy_sz);

    // Запишем настоящий размер
    *addrlen = sizeof(struct sockaddr_in);

    return 0;
}

int	sock_tcp4_shutdown(struct socket *sock, int how)
{
    int shut_read = (how == SHUT_RD || how == SHUT_RDWR);
    int shut_write = (how == SHUT_WR || how == SHUT_RDWR);

    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    if (shut_read == TRUE)
    {
        sock_data->shut_rd = TRUE;
        scheduler_wake(&sock_data->rx_blk, INT_MAX);
    }

    if (shut_write == TRUE)
    {
        // TODO: когда появится очередь отправки, реализовать отправку всей очереди  
        // FIN
        tcp_ip4_fin(sock_data);
        sock_data->shut_wr = TRUE;
    }

    return 0;
}

int sock_tcp4_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen)
{
    printk("TCP Setsockopt level:%i name:%i\n");
    return 0;
}

short sock_tcp4_poll(struct socket *sock, struct file *file, struct poll_ctl *nctl)
{
    short poll_mask = 0;
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    poll_wait(nctl, file, &sock_data->rx_poll_wq);

    if ((sock->state == SOCKET_STATE_LISTEN) && (sock_data->accept_queue.head != NULL))
        return POLLIN | POLLRDNORM;

    // Запись закрыта с обеих сторон - HUP
    if ((sock_data->shut_rd == TRUE) && (sock_data->shut_wr == TRUE))
        poll_mask |= POLLHUP;

    // Пришел FIN
    if (sock_data->shut_rd == TRUE)
        poll_mask |= POLLIN | POLLRDNORM | POLLRDHUP;

    // очередь приема не пуста?
    if (list_size(&sock_data->rx_queue) > 0)
        poll_mask |= (POLLIN | POLLRDNORM);

    // Пока что запись возможна, когда сокет соединен
    // Это также позволяет асинхронно ожидать connect()
    if (sock->state == SOCKET_STATE_CONNECTED)
        poll_mask |= (POLLOUT | POLLWRNORM);
    
    return poll_mask;
}

void tcp_ip4_init()
{
    // Генерация ключа для SYN Cookie
    siphash_keygen(&syncookie_key);
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