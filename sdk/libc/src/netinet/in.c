#include "netinet/in.h"

uint32_t ntohl(uint32_t nb) 
{
#ifdef __LITTLE_ENDIAN__
    return ((nb >> 24) & 0xff) | ((nb << 8) & 0xff0000) |
        ((nb >> 8) & 0xff00) | ((nb << 24) & 0xff000000);
#else
    return nb;
#endif
}

uint16_t ntohs(uint16_t val) {
#ifdef __LITTLE_ENDIAN__
    return (val >> 8) | (val << 8);
#else
    return val;
#endif
}

uint32_t htonl(uint32_t hb) 
{
    return ntohl(hb);
}

uint16_t htons(uint16_t hb) 
{
    return ntohs(hb);
}