#include "stdio.h"
#include "kairax.h"
#include "arpa/inet.h"
#include "string.h"
#include "errno.h"

union ip4uni {
    uint32_t val;
    uint8_t array[4];
};

int main(int argc, char** argv) 
{
    int rc = 0;

    if (argc < 2) {
        // Просто вывод маршрутов
        struct RouteInfo4 info; 
        uint32_t index = 0;
        
        union ip4uni dest;
        union ip4uni mask;
        union ip4uni gw;

        while ((rc = RouteGet4(index ++, &info)) == 0) 
        {    
            dest.val = info.dest;
            mask.val = info.netmask;
            gw.val = info.gateway;  

            printf("%i.%i.%i.%i  %i.%i.%i.%i  %i.%i.%i.%i  %i  %i   %s\n",
                dest.array[0], dest.array[1], dest.array[2], dest.array[3],
                mask.array[0], mask.array[1], mask.array[2], mask.array[3],
                gw.array[0], gw.array[1], gw.array[2], gw.array[3],
                info.flags, info.metric, info.nic_name);
        }

        return 0;
    }

    char* cmd = argv[1];
    if (strcmp(cmd, "add") == 0) 
    {
        struct RouteInfo4 info;
        memset(&info, 0, sizeof(struct RouteInfo4));
        info.gateway = inet_addr("10.0.2.2");
        strcpy(info.nic_name, "eth0");

        rc = RouteAdd4(&info);
        if (rc != 0) {
            printf("Add route failed: %i\n", errno);
            return 1;
        }
    }


    return 0;
}