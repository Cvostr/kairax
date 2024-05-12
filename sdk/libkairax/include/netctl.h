#ifndef _NETCTL_H
#define _NETCTL_H

#include "stdint.h"
#include "stddef.h"

#define MAC_DEFAULT_LEN 6
#define NIC_NAME_LEN    12

#define IF_INDEX_UNKNOWN        -1

#define OP_GET_ALL              1
#define OP_SET_IPV4_ADDR        2
#define OP_SET_IPV4_SUBNETMASK  3
#define OP_SET_IPV4_GATEWAY     4
#define OP_SET_IPV6_ADDR        5
#define OP_SET_IPV6_GATEWAY     6
#define OP_UPDATE_FLAGS         7

struct netinfo {
    char        nic_name[NIC_NAME_LEN];
    uint32_t    flags;

    uint8_t     mac[MAC_DEFAULT_LEN];
    size_t      mtu;

    uint32_t    ip4_addr;
    uint32_t    ip4_subnet;
    uint32_t    ip4_gateway;

    uint16_t    ip6_addr[8];
    uint16_t    ip6_gateway[8];
};

int netctl(int op, int param, struct netinfo* netinfo);

#endif