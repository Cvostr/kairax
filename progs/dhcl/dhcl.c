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
#define DHCP_OPT_LEASE_TIME     51
#define DHCP_OPT_MESSAGE_TYPE   53

#define DHCP_DISCOVER       1
#define DHCP_OFFER          2

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

int main(int argc, char** argv) 
{
    srand(time(NULL));

    if (argc < 2)
    {
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
    struct ether_header* eth_header = packet;
    struct ip* ip_header = eth_header + 1;
    struct udphdr* udp_header = ip_header + 1;
    struct dhcpmessage* dhcp_header = udp_header + 1;

    // Заполнение DHCP
    struct dhcpmessage dhcpmsg;
    int msgID = rand();
    bzero(&dhcpmsg, sizeof(dhcpmsg));
    dhcpmsg.op = DHCP_BOOTREQUEST;
    dhcpmsg.htype = DHCP_HTYPE_ETHERNET;
    dhcpmsg.hlen = 6;
    dhcpmsg.hops = 0;
    dhcpmsg.xid = htonl(msgID);
    dhcpmsg.secs = htons(0);
    dhcpmsg.flags = htons(0x8000);
    // Копирование MAC в запрос
    memcpy(dhcpmsg.chaddr, &ninfo.mac, 6);
    // Magic
    dhcpmsg.magic[0] = 99;
    dhcpmsg.magic[1] = 130;
    dhcpmsg.magic[2] = 83;
    dhcpmsg.magic[3] = 99;
    // Опции
    dhcpmsg.opt[0] = DHCP_OPT_MESSAGE_TYPE;
    dhcpmsg.opt[1] = 1;
    dhcpmsg.opt[2] = DHCP_DISCOVER;
    dhcpmsg.opt[3] = 0xFF;
    // Длина запроса
    int request_len = sizeof(dhcpmsg) + 4;
    // Копирование 
    memcpy(dhcp_header, &dhcpmsg, request_len);


    // --- Заполнение UDP заголовка
    udp_header->source = htons(DHCP_CLIENT_PORT);
    udp_header->dest = htons(DHCP_SERVER_PORT);
    udp_header->len = htons(sizeof(struct udphdr) + request_len);
    udp_header->check = udp4_calc_checksum(INADDR_ANY, INADDR_BROADCAST, udp_header, dhcp_header, request_len);


    // --- Заполнение IP заголовка
    ip_header->ip_dst.s_addr = INADDR_BROADCAST;
	ip_header->ip_src.s_addr = INADDR_ANY;
    ip_header->ip_v = IPVERSION;
    ip_header->ip_hl = sizeof(struct ip) / 4; 
	ip_header->ip_p = IPPROTO_UDP;
	ip_header->ip_ttl = IPDEFTTL;
    ip_header->ip_len = htons(request_len + sizeof(struct udphdr) + sizeof(struct ip));
	ip_header->ip_sum = htons(ipv4_calculate_checksum(ip_header, sizeof(struct ip)));

    
    // Заполнение ethernet заголовка
    eth_header->ether_type = htons(ETHERTYPE_IP);
    memcpy(eth_header->ether_shost, ninfo.mac, ETHER_ADDR_LEN);
    memset(eth_header->ether_dhost, 0xFF, ETHER_ADDR_LEN);

    // Отправка
    int total_len = sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct udphdr) + request_len;
    if (sendto(sockfd, packet, total_len, 0, NULL, 0) < 0)
    {
        perror("Error sending DHCP request");
        return 1;
    }

    free(packet);

    struct dhcpmessage* recvdhcpmsg;
    int recv_len = 2048;
    char* recv_buffer = malloc(recv_len);
    struct packet_sockaddr_in recv_addr;
    socklen_t rservlen = sizeof(recv_addr);

    while (1)
    {
        // прием пакета
        if (recvfrom(sockfd, recv_buffer, recv_len, 0, (struct sockaddr*)&recv_addr, &rservlen) < 0)
        {
            perror("Error receiving DHCP response");
            return 1;
        }

        // адреса заголовков
        eth_header = recv_buffer;
        ip_header = eth_header + 1;
        udp_header = ip_header + 1;
        recvdhcpmsg = udp_header + 1;

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

        if (ntohl(recvdhcpmsg->xid) != msgID)
        {
            continue;
        }

        break;
    }

    struct in_addr offered_addr;
    offered_addr.s_addr = recvdhcpmsg->yiaddr;
    printf("Offered IP %s\n", inet_ntoa(offered_addr));

    struct dhcp_offer_opts offer_opts;
    parse_offer_opts(recvdhcpmsg, &offer_opts);

    printf("Router IP %s\n", inet_ntoa(offer_opts.router));
    printf("Netmask IP %s\n", inet_ntoa(offer_opts.subnet_mask));

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