#ifndef _NET_DEVICE_H
#define _NET_DEVICE_H

#include "kairax/types.h"

#define NIC_NAME_LEN    12
#define MAC_DEFAULT_LEN 6

#define NIC_FLAG_UP         0x1
#define NIC_FLAG_BROADCAST  0x2
#define NIC_FLAG_MULTICAST  0x4
#define NIC_FLAG_LOOPBACK   0x8

struct device;

struct nic_stats {
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

struct nic {

    struct device*  dev;
    char            name[NIC_NAME_LEN];
    uint32_t        flags;

    uint8_t         mac[MAC_DEFAULT_LEN];
    size_t          mtu;
    
    uint32_t        ipv4_addr;
    uint32_t        ipv4_subnet;
    uint32_t        ipv4_gateway;

    uint16_t        ipv6_addr[8];
    uint16_t        ipv6_gateway[8];

    struct nic_stats stats;

    int (*tx) (struct nic*, const unsigned char*, size_t);
    int (*up) (struct nic*);
    int (*down) (struct nic*);
};

struct nic* new_nic();
struct nic* get_nic(int index);
struct nic* get_nic_by_name(const char* name);
int register_nic(struct nic* nic, const char* name_prefix);

int nic_set_addr_ip4(struct nic* nic, uint32_t addr);
int nic_set_subnetmask_ip4(struct nic* nic, uint32_t subnet);
int nic_set_flags(struct nic* nic, uint32_t flags);

#endif