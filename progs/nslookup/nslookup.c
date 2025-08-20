#include <netdb.h>
#include <stdio.h>
#include "arpa/inet.h"
#include "netinet/in.h"

int main(int argc, char** argv) {

    if (argc != 2)
    {
        printf("Invalid usage\n");
        return 1;
    }

    char* addr = argv[1];
    int has_response = 0;

    // Получить для IPv4
    struct hostent* host = gethostbyname(addr);
    if (host != NULL)
    {
        printf("Addresses:\n");

        has_response = 1;
        for (int i = 0; host->h_addr_list[i] != NULL; i ++ )
        {
            struct in_addr* addr = (struct in_addr*) host->h_addr_list[i];
            printf("%s\n", inet_ntoa(*addr));
        }
    }

    // Получить для IPv6
    host = gethostbyname2(addr, AF_INET6);
    if (host != NULL)
    {
        if (has_response == 0)
        {
            printf("Addresses:\n");
        }

        has_response = 1;
        for (int i = 0; host->h_addr_list[i] != NULL; i ++ )
        {
            char buffer[100];
            char* addr = host->h_addr_list[i];
            printf("%s\n", inet_ntop(AF_INET6, addr, buffer, sizeof(buffer)));
        }
    }

    if (has_response == 0)
    {
        printf("DNS server can't find %s\n", addr);
        return 1;
    }

    return 0;
}

