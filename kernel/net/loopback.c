#include "dev/type/net_device.h"
#include "eth.h"
#include "loopback.h"

int loopback_tx(struct device* dev, const unsigned char* buffer, size_t size)
{
    // todo : плохо
    eth_handle_frame(dev, buffer, size);
    return 0;
}

void loopback_init() {
    struct nic* net_dev = new_nic();
    net_dev->dev = NULL; 
    net_dev->flags = NIC_FLAG_LOOPBACK | NIC_FLAG_UP;
    net_dev->ipv4_addr = 1ULL << 24 | 127;  // 127.0.0.1
    net_dev->ipv4_subnet = 255;
    net_dev->tx = loopback_tx;
    net_dev->mtu = 65536;
    register_nic(net_dev, "lo");
}