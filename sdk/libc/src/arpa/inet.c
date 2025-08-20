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

char* inet_fmt_ipv6(const void* src, char* buf)
{
    unsigned short* ip = (unsigned short*) src;
    char temp[10];

    buf[0] = 0;

    for (int i = 0; i < 8; i ++)
    {
        short cur = ntohs(ip[i]);

        // Добавляем слово, если оно не 0
        if (cur != 0)
        {
            sprintf(temp, "%x", cur);
            strcat(buf, temp);
        }

        // Добавляем : если
        // 1. Это не последний сегмент
        // 2. текущий сегмент или следующий не 0
        if ((i != 7) && ((cur != 0) || ip[i + 1] != 0))
        {
            size_t tlen = strlen(buf);
            buf[tlen] = ':';
            buf[tlen + 1] = '\0';
        }
    }

    return buf;
}

const char* inet_ntop (int af, const void* src, char* dst, size_t len)
{
    char buf[100];
    size_t _len;

    switch (af)
    {
    case AF_INET:
        inet_ntoa_r(*(struct in_addr*) src, buf);
        _len = strlen(buf);
        break;
    case AF_INET6:
        inet_fmt_ipv6(src, buf);
        _len = strlen(buf);
        break;
    default:
        return NULL;
    }

    if (_len < len) 
    {
        strcpy(dst, buf);
        return dst;
    }

    return NULL;
}