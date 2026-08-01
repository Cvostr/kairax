#include "stdio.h"
#include "kairax.h"
#include "arpa/inet.h"
#include "string.h"
#include "stdlib.h"
#include "errno.h"

union ip4uni {
    uint32_t val;
    uint8_t array[4];
};

// route add 10.10.0.0 netmask 255.255.0.0 gw 192.168.0.1 metric 1 dev eth0
int route_add(int argc, char **argv)
{
    char *dest = NULL;
    char *netmask = NULL;
    char *gw = NULL;
    char *metric = NULL;
    char *dev = NULL;

    // Куда будем все заполнять
    struct RouteInfo4 info;
    info.metric = 0;
    info.flags = 0;

    for (int i = 0; i < argc; i ++)
    {
        if (i == 0)
        {
            dest = argv[0];
            continue;
        }

        char *param = argv[i++];

        if (i >= argc) {
            printf("route: missing argument\n");
            return 1;
        }

        char *arg = argv[i];
        if (strcmp(param, "netmask") == 0)
            netmask = arg;
        else if (strcmp(param, "gw") == 0)
            gw = arg;
        else if (strcmp(param, "metric") == 0)
            metric = arg;
        else if (strcmp(param, "dev") == 0)
            dev = arg;
        else 
        {
            printf("route: invalid param: %s\n", param);
            return 1;
        }
    }

    // обработка адреса назначения
    if (dest != NULL)
    {
        if (strcmp(dest, "default") == 0) {
            info.dest = 0;  // маршрут по умолчанию
            info.netmask = 0;
        } else {
            info.dest = inet_addr(dest);
            if (info.dest == (in_addr_t) -1)
            {
                printf("route: invalid address: %s\n", dest);
                return 1;
            }
        }
    }
    else
    {
        printf("route: destination address is not specified\n");
        return 1;
    }

    // обработка маски подсети
    if (netmask != NULL)
    {
        info.netmask = inet_addr(netmask);
        if (info.netmask == (in_addr_t) -1)
        {
            printf("route: invalid netmask: %s\n", netmask);
            return 1;
        }
    }
    else
    {
        // если маска и не была указана, то смотрим, является ли это маршрутом по умолчанию
        if (info.dest == 0)
        {
            // если да, то маска 0.0.0.0
            info.netmask = 0;
        }
        else
        {
            // иначе считаем адрес единичным
            // маска будет 255.255.255.255
            info.netmask = 0xFFFFFFFF;
        }
    }

    // обработка шлюза
    if (gw != NULL)
    {
        info.gateway = inet_addr(gw);
        if (info.gateway == (in_addr_t) -1)
        {
            printf("route: invalid gateway: %s\n", gw);
            return 1;
        }
    }
    else
    {
        // если шлюз не указан, смотрим, указано ли устройство
        // если не указано, то ошибка
        if (dev == NULL)
        {
            printf("route: dev should be specified!");
            return 1;
        }

        // Иначе считаем, что 0.0.0.0
        info.gateway = 0;
    }

    // обработка маски подсети
    if (metric != NULL)
    {
        char *endptr;
        info.metric = strtol(metric, &endptr, 10);
    
        if (*endptr != '\0') 
        {
            printf("route: invalid metric: %s\n", metric);
            return 1;
        }
    }
    else
    {
        // если метрика не указана, то по умолчанию 0
        info.metric = 0;
    }

    if (strlen(dev) > NIC_NAME_LEN) 
    {
        printf("route: dev length is too long!");
        return 1;
    }

    strncpy(info.nic_name, dev, NIC_NAME_LEN);

    // Пробуем добавить
    int rc = RouteAdd4(&info);
    if (rc != 0) {
        printf("Add route failed: %i\n", errno);
        return 1;
    }

    return 0;
}

int main(int argc, char** argv) 
{
    int rc = 0;

    if (argc < 2) 
    {
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
        return route_add(argc - 2, argv + 2);
    }
    else
    {
        printf("route: Unknown command\n");
        return 1;
    }

    return 0;
}