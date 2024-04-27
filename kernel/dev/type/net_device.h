#ifndef _NET_DEVICE_H
#define _NET_DEVICE_H

#include "kairax/types.h"

struct net_device_info {

    unsigned char mac[6];

    int (*tx) (struct device*, const unsigned char*, size_t);
};

#endif