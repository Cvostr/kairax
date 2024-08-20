#ifndef _IN_H
#define _IN_H

#include "kairax/types.h"

static inline uint16_t ntohs(uint16_t val) {
#ifdef __LITTLE_ENDIAN__
    return (val >> 8) | (val << 8);
#else
    return val;
#endif
}
   
static inline uint32_t ntohl(uint32_t nb) 
{
#ifdef __LITTLE_ENDIAN__
    return ((nb >> 24) & 0xff) | ((nb << 8) & 0xff0000) |
        ((nb >> 8) & 0xff00) | ((nb << 24) & 0xff000000);
#else
    return nb;
#endif
}

static inline uint16_t htons(uint16_t val)
{
    return ntohs(val);
} 

static inline uint32_t htonl(uint32_t val)
{
    return ntohl(val);
} 

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
};

#endif