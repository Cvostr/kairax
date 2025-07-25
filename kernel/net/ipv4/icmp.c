#include "kairax/types.h"
#include "icmp.h"
#include "ipv4.h"
#include "kairax/string.h"
#include "stdio.h"

//#define ICMP_LOGGING

struct ip4_protocol ip4_icmp_protocol = {
    .handler = icmp_ip4_handle
};

int icmp_ip4_handle(struct net_buffer* nbuffer)
{
    struct icmp_header* icmph = (struct icmp_header*) nbuffer->transp_header;

#ifdef ICMP_LOGGING
    printk("ICMP packet received, type: %i\n", icmph->type);
#endif

    if (icmp_checksum(icmph, nbuffer->netw_packet_size) != 0)
    {
        printk("ICMP: checksum invalid\n");
    }

    net_buffer_acquire(nbuffer);

    if (icmph->type == ICMP_ECHO) 
    {
        icmp_ip4_handle_ping(nbuffer);
    }

    net_buffer_free(nbuffer);
    
    return 0;
}

void icmp_ip4_handle_ping(struct net_buffer* nbuffer)
{
    struct icmp_header* icmph = (struct icmp_header*) nbuffer->transp_header;
    struct ip4_packet* ip4p = (struct ip4_packet*) nbuffer->netw_header;

    struct icmp_header response_header;
    memset(&response_header, 0, sizeof(response_header));
    response_header.type = ICMP_ECHOREPLY;
    response_header.un.echo.id = icmph->un.echo.id;
    response_header.un.echo.sequence = icmph->un.echo.sequence;
    response_header.checksum = icmp_checksum(&response_header, sizeof(response_header));

    // Маршрут назначения
    struct route4* route = route4_resolve(ip4p->src_ip);
    if (route == NULL)
    {
        printk("ICMP: NO ROUTE!!!\n");
        return;
    }

    struct net_buffer* resp = new_net_buffer_out(256);
    resp->netdev = route->interface;
    net_buffer_acquire(resp);

    // Добавляем UDP заголовок к буферу
    net_buffer_add_front(resp, &response_header, sizeof(response_header));

    int rc = ip4_send(resp, route, ip4p->src_ip, IPV4_PROTOCOL_ICMP);
 
    // освобождение памяти, выделенной под буфер
    net_buffer_free(resp);
}

uint16_t icmp_checksum(void *b, int len) 
{
    uint16_t *buf = b;
    unsigned int sum = 0;
    uint16_t result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
        
    if (len == 1)
        sum += *(unsigned char *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;

    return result;
}

void icmp_ip4_init()
{
    ip4_register_protocol(&ip4_icmp_protocol, IPV4_PROTOCOL_ICMP);
}