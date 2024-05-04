#include "stdio.h"
#include "netctl.h"
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

        while ((rc = netctl(OP_GET_ALL, i++, &ninfo)) == 0) {

            printf("%s: mtu %i\n", ninfo.nic_name, ninfo.mtu);
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

        }
        
    } else if (argc > 3) {
        char* ifname = argv[1];
        char* action = argv[2];

        strncpy(ninfo.nic_name, ifname, NIC_NAME_LEN);
        if (strcmp(action, "setip4") == 0) {
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
        }

    } else {
        printf("Bad Command!\n");
    }

    return 0;
}