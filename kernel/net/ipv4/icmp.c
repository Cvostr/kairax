#include "kairax/types.h"
#include "icmp.h"
#include "ipv4.h"

//#define ICMP_LOGGING

struct ip4_protocol ip4_icmp_protocol = {
    .handler = icmp_ip4_handle
};

int icmp_ip4_handle(struct net_buffer* nbuffer)
{
    struct icmp_header* icmph = nbuffer->transp_header;

#ifdef ICMP_LOGGING
    printk("ICMP packet received, type: %i\n", icmph->type);
#endif

    if (icmp_checksum(icmph, nbuffer->netw_packet_size) != 0)
    {
        printk("ICMP: checksum invalid\n");
    }

    net_buffer_acquire(nbuffer);
    net_buffer_free(nbuffer);
    
    return 0;
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