#ifndef _GUID_H
#define _GUID_H

#include "stdint.h"

typedef struct PACKED {
    uint32_t    d1;
    uint16_t    d2;
    uint16_t    d3;
    uint64_t    d4 : 16;
    uint64_t    d5 : 48;
} guid_t;

int guid_cmp(guid_t* a, guid_t* b);

int guid_is_zero(guid_t* a);

#endif