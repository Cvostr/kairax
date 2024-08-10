#include "tcp.h"
#include "ipv4.h"
#include "kairax/in.h"
#include "kairax/errors.h"
#include "mem/kheap.h"
#include "proc/thread_scheduler.h"
#include "string.h"
#include "drivers/char/random.h"

struct ip4_protocol ip4_tcp_protocol = {
    .handler = tcp_ip4_handle
};

struct socket* tcp4_bindings[65536];

int tcp_ip4_handle(struct net_buffer* nbuffer)
{
    struct tcp_packet* tcp_packet = (struct tcp_packet*) nbuffer->cursor;
    nbuffer->transp_header = tcp_packet;

    uint16_t flags = ntohs(tcp_packet->hlen_flags);
    flags &= 0b111111111;
    uint16_t src_port = ntohs(tcp_packet->src_port);
    uint16_t dst_port = ntohs(tcp_packet->dst_port);

    printk("Src port: %i, dest port: %i, fl: %i\n", src_port, dst_port, flags);

    struct socket* sock = tcp4_bindings[dst_port];
    if (sock == NULL)
    {
        // ??? - реализовать
        return -1;
    }

    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;

    if ((flags & TCP_FLAG_SYN) == TCP_FLAG_SYN) {

        if (sock->state != SOCKET_STATE_LISTEN) {
            // ? CONNREFUSED?
            return 1;
        }

        // запрос соединения
        if (sock_data->backlog_tail == sock_data->backlog_sz) {
            // ? CONNREFUSED?
            return 1;
        }

        // Добавляем запрос к очереди подключений
        net_buffer_acquire(nbuffer);
        sock_data->backlog[sock_data->backlog_tail] = nbuffer;
        sock_data->backlog_tail++;

        scheduler_wakeup(sock_data->backlog, 1);
    }
}

struct socket_prot_ops ipv4_stream_ops = {
    .create = sock_tcp4_create,
    .connect = sock_tcp4_connect,
    .accept = sock_tcp4_accept,
    .bind = sock_tcp4_bind,
    .listen = sock_tcp4_listen,
    .recvfrom = sock_tcp4_recvfrom,
    .sendto = sock_tcp4_sendto
};

int sock_tcp4_create (struct socket* sock)
{
    sock->data = kmalloc(sizeof(struct tcp4_socket_data));
    memset(sock->data, 0, sizeof(struct tcp4_socket_data));
    return 0;
}

int	sock_tcp4_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len)
{
    return 0;   
}

int	sock_tcp4_accept(struct socket *sock, struct socket **newsock, struct sockaddr *addr)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    while (sock_data->backlog_head == sock_data->backlog_tail - 1) 
    {
        scheduler_sleep(sock_data->backlog, NULL);
    }

    struct sockaddr_in client_addr;
    struct net_buffer* syn_req = sock_data->backlog[++sock_data->backlog_head];
    struct tcp_packet* tcpp = syn_req->transp_header;
    struct ip4_packet* ip4p = syn_req->netw_header;

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = ip4p->src_ip;
    client_addr.sin_port = ntohs(tcpp->src_port);

    // Создание сокета клиента
    struct socket* client_sock = new_socket();
    struct tcp4_socket_data* client_sockdata = kmalloc(sizeof(struct tcp4_socket_data));
    memset(client_sockdata, 0, sizeof(struct tcp4_socket_data));

    client_sock->data = client_sockdata;

    // Биндинг сокета клиента
    memcpy(&client_sockdata->addr, &client_addr, sizeof(struct sockaddr_in));
    tcp4_bindings[client_addr.sin_port] = client_sock;

    union ip4uni src;
	src.val = client_addr.sin_addr.s_addr; 
	printk("Accepted client: IP4 : %i.%i.%i.%i, port: %i,\n", src.array[0], src.array[1], src.array[2], src.array[3], client_addr.sin_port);

    struct net_buffer* resp = new_net_buffer_out(4096);

    int flags = (TCP_FLAG_SYN | TCP_FLAG_ACK);

    // Генерируем начальный SN
    client_sockdata->sn = krand();
    client_sockdata->ack = ntohl(tcpp->sn);

    struct tcp_packet pkt = {0};
    pkt.src_port = htons(sock_data->addr.sin_port);
    pkt.dst_port = htons(client_sockdata->addr.sin_port);
    pkt.ack = ntohl(client_sockdata->ack + 1);
    pkt.sn = ntohl(client_sockdata->sn);
    pkt.urgent_point = 0;
    pkt.window_size = htons(65535);
    pkt.hlen_flags = htons(flags | 0x5000);
    pkt.checksum = 0;

    struct tcp_checksum_proto checksum_struct = {0};
    checksum_struct.dest = ip4p->src_ip;
    checksum_struct.src = ip4p->dst_ip;
    checksum_struct.prot = IPV4_PROTOCOL_TCP;
    checksum_struct.len = htons(sizeof(struct tcp_packet));
    pkt.checksum = htons(tcp_ip4_calc_checksum(&checksum_struct, &pkt, NULL, 0));

    // Добавить заголовок TCP к буферу  ответа
    net_buffer_add_front(resp, &pkt, sizeof(struct tcp_packet));
    
    // Освободить буфер с запросом SYN
    net_buffer_close(syn_req);

    // Попытка отправить ответ
    int rc = ip4_send(resp, checksum_struct.dest, checksum_struct.src, IPV4_PROTOCOL_TCP);
    net_buffer_close(resp);

    // TODO: завершить handshake - ожидание ACK

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
    //printk("Binding to port %i\n", port);

    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    sock_data->addr.sin_port = port;
    tcp4_bindings[port] = sock;

    return 0;
}

int sock_tcp4_listen(struct socket* sock, int backlog)
{
    struct tcp4_socket_data* sock_data = (struct tcp4_socket_data*) sock->data;
    sock_data->backlog_sz = backlog;
    sock_data->backlog_head = -1;
    sock_data->backlog_tail = 0;
    sock_data->backlog = kmalloc(sizeof(struct net_buffer*) * backlog);

    sock->state = SOCKET_STATE_LISTEN;

    return 0;
}

int sock_tcp4_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    return 0;
}

int sock_tcp4_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    return 0;
}

void tcp_ip4_init()
{
    ip4_register_protocol(&ip4_tcp_protocol, IPV4_PROTOCOL_TCP);
}

uint16_t tcp_ip4_calc_checksum(struct tcp_checksum_proto* prot, struct tcp_packet* header, unsigned char* payload, size_t payload_size)
{
    uint32_t sum = 0;
    uint16_t* data = (uint16_t*) prot;

    for (int i = 0; i < 6; ++i) {
		sum += ntohs(data[i]);
		if (sum > 0xFFFF) {
			sum = (sum >> 16) + (sum & 0xFFFF);
		}
	}

	data = (uint16_t*) header;
	for (int i = 0; i < 10; ++i) {
		sum += ntohs(data[i]);
		if (sum > 0xFFFF) {
			sum = (sum >> 16) + (sum & 0xFFFF);
		}
	}

	uint16_t d_words = payload_size / 2;

	data = (uint16_t*) payload;
	for (unsigned int i = 0; i < d_words; ++i) {
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

	return ~(sum & 0xFFFF) & 0xFFFF;
}