#include "dev/type/net_device.h"
#include "eth.h"
#include "loopback.h"
#include "net/route.h"
#include "mem/kheap.h"

#define LOOPBACK_DEFAULT_METRIC 331

int loopback_tx(struct nic* nic, const unsigned char* buffer, size_t size)
{
    // todo : плохо
    struct net_buffer* nb = new_net_buffer(buffer, size, nic);
    eth_handle_frame(nb);
    return 0;
}

int loopback_up(struct nic* nic)
{
    return 0;
}

int loopback_down(struct nic* nic)
{
    return 0;
}

void loopback_init() 
{
    struct nic* net_dev = new_nic();
    net_dev->dev = NULL; 
    net_dev->flags = NIC_FLAG_LOOPBACK | NIC_FLAG_UP;
    net_dev->ipv4_addr = 1ULL << 24 | 127;  // 127.0.0.1
    net_dev->ipv4_subnet = 255;
    net_dev->tx = loopback_tx;
    net_dev->up = loopback_up;
    net_dev->down = loopback_down;
    net_dev->mtu = 65536;
    register_nic(net_dev, "lo");

    struct route4* loopback_route = new_route4();
    loopback_route->dest = net_dev->ipv4_addr;
    loopback_route->netmask = 0xFFFFFFFF;
    loopback_route->interface = net_dev;
    loopback_route->metric = LOOPBACK_DEFAULT_METRIC;
    route4_add(loopback_route);
}