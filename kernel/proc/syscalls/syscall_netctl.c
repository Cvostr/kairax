#include "proc/syscalls.h"
#include "dev/device.h"
#include "dev/type/net_device.h"
#include "kairax/errors.h"
#include "kairax/string.h"
#include "cpu/cpu_local.h"

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

int sys_netctl(int op, int param, struct netinfo* netinfo)
{
    // проверить память netinfo
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, netinfo, sizeof(struct netinfo))
    int rc = 0;

    // получить объект nic по имени или индексу
    struct nic* nic = NULL;
    if (param == IF_INDEX_UNKNOWN) {
        nic = get_nic_by_name(netinfo->nic_name);
    } else {
        nic = get_nic(param);
    }

    if (nic == NULL) {
        return -ERROR_NO_FILE;
    }
    
    switch (op) {
        case OP_GET_ALL:
            if (nic->dev != NULL) {
                //memcpy(&netinfo->dev_id, &nic->dev->id, sizeof(guid_t));
            }

            strcpy(netinfo->nic_name, nic->name);
            memcpy(netinfo->mac, nic->mac, MAC_DEFAULT_LEN);

            netinfo->flags = nic->flags;
            netinfo->mtu = nic->mtu;
            netinfo->ip4_addr = nic->ipv4_addr;
            netinfo->ip4_subnet = nic->ipv4_subnet;
            netinfo->ip4_gateway = nic->ipv4_gateway;
            break;
        case OP_SET_IPV4_ADDR:
            rc = nic_set_addr_ip4(nic, netinfo->ip4_addr);
            break;
        case OP_SET_IPV4_SUBNETMASK:
            rc = nic_set_subnetmask_ip4(nic, netinfo->ip4_subnet);
            break;
        case OP_UPDATE_FLAGS:
            rc = nic_set_flags(nic, netinfo->flags);
            break;
    }

    return rc;
}

int sys_netstat(int index, struct netstat* stat)
{
    // проверить память netstat
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, stat, sizeof(struct nic_stats))

    // получить объект nic по имени или индексу
    struct nic* nic = NULL;
    if (index == IF_INDEX_UNKNOWN) {
        return -EINVAL;
    } else {
        nic = get_nic(index);
    }

    if (nic == NULL) {
        return -ERROR_NO_FILE;
    }

    // Получение данных
    memcpy(stat, &nic->stats, sizeof(struct nic_stats));

    return 0;
}