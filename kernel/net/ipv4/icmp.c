#include "kairax/types.h"
#include "icmp.h"
#include "ipv4.h"

//#define ICMP_LOGGING

struct ip4_protocol ip4_icmp_protocol = {
    .handler = icmp_ip4_handle
};

void icmp_ip4_handle(struct net_buffer* nbuffer)
{
#ifdef ICMP_LOGGING
    printk("ICMP packet received\n");
#endif
    net_buffer_acquire(nbuffer);
    net_buffer_close(nbuffer);
}

void icmp_ip4_init()
{
    ip4_register_protocol(&ip4_icmp_protocol, IPV4_PROTOCOL_ICMP);
}