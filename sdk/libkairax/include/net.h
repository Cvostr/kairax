#ifndef _NETCTL_H
#define _NETCTL_H

#include "stdint.h"
#include "stddef.h"
#include "defs.h"

#define OP_GET_ALL              1
#define OP_SET_IPV4_ADDR        2
#define OP_SET_IPV4_SUBNETMASK  3
#define OP_SET_IPV4_GATEWAY     4
#define OP_SET_IPV6_ADDR        5
#define OP_SET_IPV6_GATEWAY     6
#define OP_UPDATE_FLAGS         7

#define NIC_FLAG_UP         0x1
#define NIC_FLAG_BROADCAST  0x2
#define NIC_FLAG_MULTICAST  0x4
#define NIC_FLAG_LOOPBACK   0x8
#define NIC_FLAG_NO_CARRIER 0x10

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

struct netstat {
    uint64_t        rx_packets;
    uint64_t        rx_bytes;
    uint64_t        rx_errors;
    uint64_t        rx_frame_errors;
    uint64_t        rx_overruns;
    uint64_t        rx_dropped;

    uint64_t        tx_packets;
    uint64_t        tx_bytes;
    uint64_t        tx_errors;
    uint64_t        tx_dropped;
    uint64_t        tx_carrier;
};

int netctl(int op, int param, struct netinfo* netinfo);
#define NetCtl netctl

int netstat(int index, struct netstat* stat);
#define GetNetInterfaceStat netstat

#endif