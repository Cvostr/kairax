#include "arpa/inet.h"
#include "errno.h"

int inet_pton(int af, const char* src, void* dst)
{
    switch (af)
    {
    case AF_INET:
        if (!inet_aton(src, dst))
            return 0;
        break;
    case AF_INET6:
        // TODO: Реализовать
        errno = EAFNOSUPPORT;
        return -1;
    default:
        errno = EAFNOSUPPORT;
        return -1;
    }

    return 1;
}