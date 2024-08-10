#ifndef _ROUTE_H
#define _ROUTE_H

#include "kairax/types.h"

struct nic;

struct route4 {
    uint32_t    dest;
    uint32_t    netmask;
    uint32_t    gateway;
    uint16_t    flags;
    uint32_t    metric;
    struct nic* interface;
};

struct route4* route4_get(uint32_t index);
struct route4* route4_resolve(uint32_t dest);
int route4_add(struct route4* route);

#endif