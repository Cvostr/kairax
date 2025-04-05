#include "proc/syscalls.h"
#include "dev/device.h"
#include "dev/type/net_device.h"
#include "kairax/errors.h"
#include "kairax/string.h"
#include "cpu/cpu_local.h"

#include "net/route.h"
#include "mem/kheap.h"

struct route_info;

struct route4_info {
    uint32_t    dest;
    uint32_t    netmask;
    uint32_t    gateway;
    uint16_t    flags;
    uint32_t    metric;
    char        nic_name[NIC_NAME_LEN];
};

#define ROUTECTL_ACTION_ADD 1
#define ROUTECTL_ACTION_DEL 2
#define ROUTECTL_ACTION_GET 2

int routectl4(int action, int arg, struct route4_info* route);

int sys_routectl(int type, int action, int arg, struct route_info* route)
{
    switch (type) {
        case 4:
            return routectl4(action, arg, (struct route4_info*) route);
        default:
            return -EINVAL;
    }

    return -EINVAL;
}

int routectl4(int action, int arg, struct route4_info* route)
{
    // проверить память netinfo
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, route, sizeof(struct route4_info))

    struct nic* nic = NULL;

    if (route->nic_name[0] != 0)
        nic = get_nic_by_name(route->nic_name);

    int is_default_route = route->dest == 0 && route->netmask == 0;

    switch (action) {
        case ROUTECTL_ACTION_GET:
            struct route4* route4 = route4_get(arg);
            if (route4 == NULL) {
                return -ENOENT;
            }

            route->dest = route4->dest;
            route->netmask = route4->netmask;
            route->gateway = route4->gateway;
            route->flags = route4->flags;
            route->metric = route4->metric;
            strcpy(route->nic_name, route4->interface->name);

            break;
        case ROUTECTL_ACTION_ADD:
            if (is_default_route && nic == NULL)
                return -EINVAL;

            struct route4* new_route = new_route4();
            new_route->dest = route->dest;
            new_route->netmask = route->netmask;
            new_route->gateway = route->gateway;
            new_route->flags = route->flags;
            new_route->metric = route->metric;
            new_route->interface = nic;
            route4_add(new_route);
            break;
    }

    return 0;
}