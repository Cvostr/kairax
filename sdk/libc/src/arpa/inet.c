#include "arpa/inet.h"
#include <stdlib.h>

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
        }
    }

    inp->s_addr = htonl(ip);

    return 0;
}

unsigned long int inet_addr(const char* cp)
{
    struct in_addr inaddr;

    if (inet_aton(cp, &inaddr) != 0) {
        return inaddr.s_addr;
    }

    return (unsigned long long) -1;
}