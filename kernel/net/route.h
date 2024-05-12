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

struct route4* route4_resolve(uint32_t dest);

#endif