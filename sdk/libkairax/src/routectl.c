#include "routectl.h"
#include "ksyscalls.h"
#include "errno.h"

int RouteGet4(uint32_t index, struct RouteInfo4* route)
{
    return Route4(ROUTECTL_ACTION_GET, index, route);
}

int RouteAdd4(struct RouteInfo4* route)
{
    return Route4(ROUTECTL_ACTION_ADD, 0, route);
}

int Route4(int action, uint32_t arg, struct RouteInfo4* route)
{
    __set_errno( syscall_routectl(4, action, arg, route));
}