#include "stdio.h"
#include "net.h"
#include "string.h"

union ip4uni {
    uint32_t val;
    uint8_t array[4];
};

void print_ipv4(uint32_t val) 
{
    union ip4uni addr;
    addr.val = val;

    printf("%i.%i.%i.%i", addr.array[0], addr.array[1], addr.array[2], addr.array[3]);
}

int main(int argc, char** argv) 
{
    int rc = 0;
    int i = 0;
    struct netinfo ninfo = {0,};
    union ip4uni ip4addr;

    if (argc == 1) {

        while ((rc = netctl(OP_GET_ALL, i, &ninfo)) == 0) {

            printf("%s: mtu %i flags %i\n", ninfo.nic_name, ninfo.mtu, ninfo.flags  );
            printf("\tIPv4 ");
            print_ipv4(ninfo.ip4_addr);
            printf(" netmask ");
            print_ipv4(ninfo.ip4_subnet);
            printf(" gateway ");
            print_ipv4(ninfo.ip4_gateway);
            printf("\n");

            printf("\tIPv6 %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n",  ninfo.ip6_addr[0],
                                                                        ninfo.ip6_addr[1],
                                                                        ninfo.ip6_addr[2],
                                                                        ninfo.ip6_addr[3],
                                                                        ninfo.ip6_addr[4],
                                                                        ninfo.ip6_addr[5],
                                                                        ninfo.ip6_addr[6],
                                                                        ninfo.ip6_addr[7]);
                                                                        
            printf("\tMAC %02x:%02x:%02x:%02x:%02x:%02x\n", ninfo.mac[0],
                                                            ninfo.mac[1],
                                                            ninfo.mac[2],
                                                            ninfo.mac[3],
                                                            ninfo.mac[4],
                                                            ninfo.mac[5]);

            struct netstat stat;
            GetNetInterfaceStat(i, &stat);
            
            printf("\tRX packets %i bytes %i\n", stat.rx_packets, stat.rx_bytes);
            printf("\tRX errors %i dropped %i overrun %i frame %i\n", stat.rx_errors, stat.rx_dropped, stat.rx_overruns, stat.rx_frame_errors);
            printf("\tTX packets %i bytes %i\n", stat.tx_packets, stat.tx_bytes);
            printf("\tTX errors %i dropped %i carrier %i\n", stat.tx_errors, stat.tx_dropped, stat.tx_carrier);

            i++;
        }
        
    } else if (argc >= 3) {
        char* ifname = argv[1];
        char* action = argv[2];

        strncpy(ninfo.nic_name, ifname, NIC_NAME_LEN);
        if (strcmp(action, "setip4") == 0) {
            if (argc < 4) {
                printf("Address not specified!\n");
                return -1;
            }
            // Устанавливаем IPv4
            int ip4[4];
            sscanf(argv[3], "%i.%i.%i.%i", &ip4[0], &ip4[1], &ip4[2], &ip4[3]);

            for (int i = 0; i < 4; i ++) {
                if (ip4[i] > 255) {
                    printf("Invalid IP address\n");
                    return 1;
                }
                ip4addr.array[i] = ip4[i];
            }

            ninfo.ip4_addr = ip4addr.val;
            rc = netctl(OP_SET_IPV4_ADDR, -1, &ninfo);
            if (rc != 0) {
                printf("Error on netctl(), rc = %i\n", rc);
            }
        } else if (strcmp(action, "up") == 0) {

            // Получить текущие флаги устройства
            rc = netctl(OP_GET_ALL, -1, &ninfo);
            ninfo.flags |= 1;
            // Обновить флаги
            rc = netctl(OP_UPDATE_FLAGS, -1, &ninfo);
        } else if (strcmp(action, "down") == 0) {
            // Получить текущие флаги устройства
            rc = netctl(OP_GET_ALL, -1, &ninfo);
            ninfo.flags &= ~(1);
            // Обновить флаги
            rc = netctl(OP_UPDATE_FLAGS, -1, &ninfo);
        }

    } else {
        printf("Bad Command!\n");
    }

    return 0;
}