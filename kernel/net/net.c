#include "loopback.h"

void net_init() 
{
    loopback_init();
    local_sock_init();
    ipv4_sock_init();

    udp_ip4_init();
    tcp_ip4_init();
}