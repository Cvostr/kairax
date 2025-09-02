#include "net.h"
#include "loopback.h"
#include "ipv4/tcp.h"
#include "ipv4/udp.h"
#include "ipv4/icmp.h"
#include "ipv4/af_inet.h"
#include "packet/af_packet.h"

void net_init() 
{
    loopback_init();
    local_sock_init();
    ipv4_sock_init();

    udp_ip4_init();
    tcp_ip4_init();
    icmp_ip4_init();

    packet_sock_init();
}