#include "arpa/inet.h"
#include <stdlib.h>
#include "stdio.h"
#include "string.h"

int inet_aton(const char* cp, struct in_addr* inp)
{
    uint32_t ip = 0;
    char* paddr = (char*) cp;

    while (*paddr != 0)
    {
        int num = strtoul(paddr, &paddr, 10);
        ip <<= 8;
        ip |= num;

        if (*paddr == '.')
        {
            if (num > 255) {
                return 0;
            }
            paddr++;
        } else if (*paddr == '\0') {
            break;
        } else {
            return 0;
        }
    }

    inp->s_addr = htonl(ip);

    return 1;
}

in_addr_t inet_addr(const char* cp)
{
    struct in_addr inaddr;

    if (inet_aton(cp, &inaddr) != 0) {
        return inaddr.s_addr;
    }

    return (in_addr_t) -1;
}

char* inet_ntoa(struct in_addr in)
{
    static char buf[20];
    return inet_ntoa_r(in, buf);
}

char* inet_ntoa_r(struct in_addr in, char* buf)
{
    unsigned char* ip = (unsigned char*) &in;
    sprintf(buf, "%i.%i.%i.%i", ip[0], ip[1], ip[2], ip[3]);
    return buf;
}