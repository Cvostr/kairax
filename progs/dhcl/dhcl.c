#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include "defs.h"
#include "kairax.h"

struct dhcpmessage
{
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    char chaddr[16];
    char sname[64];
    char file[128];
    char magic[4];
    char opt[];
} __attribute__((__packed__));

struct dhcp_offer_opts {
    struct in_addr router;
    struct in_addr subnet_mask;
    struct in_addr ns_serv;
    uint32_t lease_time;
};

struct packet_sockaddr_in {  
    sa_family_t sin_family;
    char ifname[NIC_NAME_LEN];
};

#define DHCP_SERVER_PORT 67 
#define DHCP_CLIENT_PORT 68

#define DHCP_BOOTREQUEST    1

#define DHCP_HTYPE_ETHERNET 1

#define DHCP_OPT_SUBNET_MASK    1
#define DHCP_OPT_ROUTER         3
#define DHCP_OPT_HOSTNAME       12
#define DHCP_OPT_REQUESTED_IP   50
#define DHCP_OPT_LEASE_TIME     51
#define DHCP_OPT_MESSAGE_TYPE   53

#define DHCP_DISCOVER       1
#define DHCP_OFFER          2
#define DHCP_REQUEST        3
#define DHCP_ACK            5

uint16_t udp4_calc_checksum(uint32_t src, uint32_t dest, struct udphdr* header, const unsigned char* payload, size_t payload_size)
{
    uint16_t* data;
    int i = 0;
    uint32_t sum = 0;
    uint32_t header_sz = sizeof(struct udphdr) / sizeof(uint16_t);

    // Псевдозаголовок
    sum += (src >> 16) & 0xFFFF;
    sum += (src) & 0xFFFF;

    sum += (dest >> 16) & 0xFFFF;
    sum += (dest) & 0xFFFF;

    sum += htons(IPPROTO_UDP);
    sum += htons(sizeof(struct udphdr) + payload_size);

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

uint16_t ipv4_calculate_checksum(void* data, size_t len)
{
    uint32_t sum = 0;
	struct ip* pck = ((struct ip*) (data));
	uint16_t* s = data;
	// Для вычисления настоящей - временно обнулим поле в пакете
	pck->ip_sum = 0;

	for (int i = 0; i < len / 2; ++i) {
		sum += ntohs(s[i]);
		if (sum > 0xFFFF) {
			sum = (sum >> 16) + (sum & 0xFFFF);
		}
	}

	return ~(sum & 0xFFFF) & 0xFFFF;
}


void parse_offer_opts(struct dhcpmessage* offer, struct dhcp_offer_opts* out)
{
    uint8_t opt_type;
    uint8_t opt_len;

    uint8_t* cur = offer->opt;

    while (*cur != 0xFF)
    {
        opt_type = *cur;
        cur ++;
        opt_len = *cur;
        cur ++;

        switch (opt_type)
        {
            case DHCP_OPT_SUBNET_MASK:
                memcpy(&out->subnet_mask, cur, opt_len);
                break;
            case DHCP_OPT_LEASE_TIME:
                memcpy(&out->lease_time, cur, opt_len);
                break;
            case DHCP_OPT_ROUTER:
                memcpy(&out->router, cur, opt_len);
                break;
        }

        cur += opt_len;
    }
}

char* dhcp_put_opt(char* opt_ptr, char opt, uint8_t len, char* value, int* msg_len)
{
    opt_ptr[0] = opt;
    opt_ptr[1] = len;
    memcpy(opt_ptr + 2, value, len);
    *msg_len += 2 + len;

    return opt_ptr + 2 + len;
}

char* dhcp_put_opt_val(char* opt_ptr, char opt, char value, int* msg_len)
{
    return dhcp_put_opt(opt_ptr, opt, 1, &value, msg_len);
}

void make_headers(char* packet, struct dhcpmessage* dhcpmsg, size_t dhcp_hdr_len, char* mac);
struct dhcpmessage* wait_for_response(int sock, char* buffer, size_t buflen, int dhcp_msg_id);

int main(int argc, char** argv) 
{
    srand(time(NULL));

    if (argc < 2)
    {
        printf("Usage:\n");
        printf("dhcl <if name>\n");
        return 1;
    }

    char* iface_name = argv[1];

    int rc;
    int sockfd;
    struct sockaddr_in servaddr, cliaddr, rservaddr;
    struct netinfo ninfo = {0,};
    struct packet_sockaddr_in raw_addr;

    raw_addr.sin_family = AF_PACKET;
    strcpy(raw_addr.ifname, iface_name);

    char* hostname = malloc(256);
    int hostname_rc = gethostname(hostname, 256);

    // Запрос информации об интерфейсе
    strcpy(ninfo.nic_name, iface_name);
    rc = NetCtl(OP_GET_ALL, -1, &ninfo);
    if (rc != 0)
    {
        perror("netctl failed");
        return 1;
    }

    printf("MAC %02x:%02x:%02x:%02x:%02x:%02x\n", ninfo.mac[0],
                                                            ninfo.mac[1],
                                                            ninfo.mac[2],
                                                            ninfo.mac[3],
                                                            ninfo.mac[4],
                                                            ninfo.mac[5]);

    if ((sockfd = socket(AF_PACKET, SOCK_RAW, ETHERTYPE_IP)) < 0)
    {
        perror("Error creating socket");
        return 1;
    }

    if (bind(sockfd, (struct sockaddr*)&raw_addr, sizeof(raw_addr)) < 0)
    {
        perror("Error socket bind()");
        return 1;
    }

    char* packet = malloc(4096);
    memset(packet, 0, 4096);

    // Указатели на все заголовки
    struct ether_header* eth_header = NULL;
    struct ip* ip_header = NULL;
    struct udphdr* udp_header = NULL;
    struct dhcpmessage* dhcp_header = NULL;

    // Заполнение DHCP Discover
    struct dhcpmessage* dhcpmsg = malloc(512);
    int msgID = rand();
    int dhcp_msg_len = sizeof(struct dhcpmessage);
    bzero(dhcpmsg, 512);
    dhcpmsg->op = DHCP_BOOTREQUEST;
    dhcpmsg->htype = DHCP_HTYPE_ETHERNET;
    dhcpmsg->hlen = 6;
    dhcpmsg->hops = 0;
    dhcpmsg->xid = htonl(msgID);
    dhcpmsg->secs = htons(0);
    dhcpmsg->flags = htons(0x8000);
    // Копирование MAC в запрос
    memcpy(dhcpmsg->chaddr, &ninfo.mac, 6);
    // Magic
    dhcpmsg->magic[0] = 99;
    dhcpmsg->magic[1] = 130;
    dhcpmsg->magic[2] = 83;
    dhcpmsg->magic[3] = 99;
    // Опции
    char* nextopt = dhcpmsg->opt;
    nextopt = dhcp_put_opt_val(nextopt, DHCP_OPT_MESSAGE_TYPE, DHCP_DISCOVER, &dhcp_msg_len);
    // добавить hostname
    if (hostname_rc == 0 && strlen(hostname) > 0)
    {
        nextopt = dhcp_put_opt(nextopt, DHCP_OPT_HOSTNAME, strlen(hostname), hostname, &dhcp_msg_len);
    }
    // На конец DHCP сообщения надо добавить 0xFF
    *nextopt = 0xFF;
    dhcp_msg_len += 1;
    
    // Сформировать остальные заголовки (UDP + IP + ETH)
    make_headers(packet, dhcpmsg, dhcp_msg_len, ninfo.mac);

    // Отправка
    printf("DHCP: Sending Discover\n");
    int total_len = sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct udphdr) + dhcp_msg_len;
    if (sendto(sockfd, packet, total_len, 0, NULL, 0) < 0)
    {
        perror("Error sending DHCP request");
        return 1;
    }

    struct dhcpmessage* recvdhcpmsg;
    int recv_len = 2048;
    char* recv_buffer = malloc(recv_len);

    // Ожидание ответа
    recvdhcpmsg = wait_for_response(sockfd, recv_buffer, recv_len, msgID);
    if (recvdhcpmsg == NULL)
    {
        return 1;
    }

    // Парсинг ответа
    struct in_addr offered_addr;
    offered_addr.s_addr = recvdhcpmsg->yiaddr;
    printf("Offered IP %s\n", inet_ntoa(offered_addr));

    struct dhcp_offer_opts offer_opts;
    parse_offer_opts(recvdhcpmsg, &offer_opts);

    // Логируем
    printf("Router IP %s\n", inet_ntoa(offer_opts.router));
    printf("Netmask IP %s\n", inet_ntoa(offer_opts.subnet_mask));

    // Подготовка запроса DHCP Request
    dhcp_msg_len = sizeof(struct dhcpmessage);
    // В принципе можно использовать старый заголовок
    nextopt = dhcpmsg->opt;
    // DHCP Request
    nextopt = dhcp_put_opt_val(nextopt, DHCP_OPT_MESSAGE_TYPE, DHCP_REQUEST, &dhcp_msg_len);
    // Желаемый IP
    nextopt = dhcp_put_opt(nextopt, DHCP_OPT_REQUESTED_IP, 4, &offered_addr.s_addr, &dhcp_msg_len);
    // добавить hostname
    if (hostname_rc == 0 && strlen(hostname) > 0)
    {
        nextopt = dhcp_put_opt(nextopt, DHCP_OPT_HOSTNAME, strlen(hostname), hostname, &dhcp_msg_len);
    }
    // На конец DHCP сообщения надо добавить 0xFF
    *nextopt = 0xFF;
    dhcp_msg_len += 1;

    // Сформировать остальные заголовки (UDP + IP + ETH)
    make_headers(packet, dhcpmsg, dhcp_msg_len, ninfo.mac);

    // Отправка
    printf("DHCP: Sending Request\n");
    total_len = sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct udphdr) + dhcp_msg_len;
    if (sendto(sockfd, packet, total_len, 0, NULL, 0) < 0)
    {
        perror("Error sending DHCP request");
        return 1;
    }

    // Освобождаем более не нужный массив
    free(packet);

    // Ожидание ответа
    recvdhcpmsg = wait_for_response(sockfd, recv_buffer, recv_len, msgID);
    if (recvdhcpmsg == NULL)
    {
        return 1;
    }

    printf("Updating %s params\n", ninfo.nic_name);
    // Ставим устройству IP адрес
    ninfo.ip4_addr = offered_addr.s_addr;
    rc = netctl(OP_SET_IPV4_ADDR, -1, &ninfo);
    if (rc != 0) {
        perror("Error setting IPv4 address");
        return 1;
    }

    // Ставим устройству Маску подсети
    ninfo.ip4_subnet = offer_opts.subnet_mask.s_addr;
    rc = netctl(OP_SET_IPV4_SUBNETMASK, -1, &ninfo);
    if (rc != 0) {
        perror("Error setting IPv4 netmask");
        return 1;
    }

    printf("Creating default route\n");
    // Создаем маршрут
    struct RouteInfo4 info;
    memset(&info, 0, sizeof(struct RouteInfo4));
    info.gateway = offer_opts.router.s_addr;
    strcpy(info.nic_name, iface_name);

    rc = RouteAdd4(&info);
    if (rc != 0) {
        perror("Add route failed");
        return 1;
    }

    return 0;
}

void make_headers(char* packet, struct dhcpmessage* dhcpmsg, size_t dhcp_hdr_len, char* mac)
{
    // Указатели на все заголовки
    struct ether_header* eth_header = (struct ether_header*) packet;
    struct ip* ip_header = (struct ip*) (eth_header + 1);
    struct udphdr* udp_header = (struct udphdr*) (ip_header + 1);
    struct dhcpmessage* dhcp_header = (struct dhcpmessage*) (udp_header + 1);

    // Скопировать DHCP запрос
    memcpy(dhcp_header, dhcpmsg, dhcp_hdr_len);

    // --- Заполнение UDP заголовка
    udp_header->source = htons(DHCP_CLIENT_PORT);
    udp_header->dest = htons(DHCP_SERVER_PORT);
    udp_header->len = htons(sizeof(struct udphdr) + dhcp_hdr_len);
    udp_header->check = 0;
    udp_header->check = udp4_calc_checksum(INADDR_ANY, INADDR_BROADCAST, udp_header, (uint8_t*) dhcp_header, dhcp_hdr_len);


    // --- Заполнение IP заголовка
    ip_header->ip_dst.s_addr = INADDR_BROADCAST;
	ip_header->ip_src.s_addr = INADDR_ANY;
    ip_header->ip_v = IPVERSION;
    ip_header->ip_hl = sizeof(struct ip) / 4; 
	ip_header->ip_p = IPPROTO_UDP;
	ip_header->ip_ttl = IPDEFTTL;
    ip_header->ip_len = htons(dhcp_hdr_len + sizeof(struct udphdr) + sizeof(struct ip));
    ip_header->ip_sum = 0;
	ip_header->ip_sum = htons(ipv4_calculate_checksum(ip_header, sizeof(struct ip)));

    
    // Заполнение ethernet заголовка
    eth_header->ether_type = htons(ETHERTYPE_IP);
    memcpy(eth_header->ether_shost, mac, ETHER_ADDR_LEN);
    memset(eth_header->ether_dhost, 0xFF, ETHER_ADDR_LEN);
}

struct dhcpmessage* wait_for_response(int sock, char* buffer, size_t buflen, int dhcp_msg_id)
{
    // Указатели на все заголовки
    struct ether_header* eth_header = NULL;
    struct ip* ip_header = NULL;
    struct udphdr* udp_header = NULL;
    struct dhcpmessage* dhcp_header = NULL;

    struct packet_sockaddr_in recv_addr;
    socklen_t rservlen = sizeof(recv_addr);

    while (1)
    {
        // прием пакета
        if (recvfrom(sock, buffer, buflen, 0, (struct sockaddr*)&recv_addr, &rservlen) < 0)
        {
            perror("Error receiving DHCP response");
            return NULL;
        }

        // адреса заголовков
        eth_header = (struct ether_header*) (buffer);
        ip_header = (struct ip*) (eth_header + 1);
        udp_header = (struct udphdr*) (ip_header + 1);
        dhcp_header = (struct dhcpmessage*) (udp_header + 1);

        if (ip_header->ip_p != IPPROTO_UDP) 
        {
			continue;
        }

        if (ntohs(udp_header->source) != DHCP_SERVER_PORT) 
        {
			continue;
        }

        if (ntohs(udp_header->dest) != DHCP_CLIENT_PORT) 
        {
			continue;
        }

        if (ntohl(dhcp_header->xid) != dhcp_msg_id)
        {
            continue;
        }

        return dhcp_header;
    }

    return NULL;
}