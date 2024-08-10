#ifndef _ROUTECTL_H
#define _ROUTECTL_H

#include "stdint.h"
#include "stddef.h"
#include "defs.h"

struct RouteInfo4 {
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

int RouteGet4(uint32_t index, struct RouteInfo4* route);
int RouteAdd4(struct RouteInfo4* route);

int Route4(int action, uint32_t arg, struct RouteInfo4* route);

#endif