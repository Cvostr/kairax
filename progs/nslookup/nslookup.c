#include <netdb.h>
#include <stdio.h>
#include "arpa/inet.h"
#include "netinet/in.h"

int find_direct(char* addr);
int find_inverse(int af, void* addr, socklen_t addrlen);

int main(int argc, char** argv) {

    if (argc != 2)
    {
        printf("Invalid usage\n");
        return 1;
    }

    char* addr = argv[1];

    struct in_addr ip4addr;

    if (inet_pton(AF_INET, addr, &ip4addr) == 1)
    {
        return find_inverse(AF_INET, &ip4addr, sizeof(struct in_addr));
    } 
    else if (inet_pton(AF_INET6, addr, &ip4addr) == 1)
    {
        return 2;
    }
    
    return find_direct(addr);
}

int find_inverse(int af, void* addr, socklen_t addrlen)
{
    struct hostent* host = gethostbyaddr(addr, addrlen, af);
    if (host == NULL)
    {
        printf("nslookup: %s\n", hstrerror(h_errno));
        return 1;
    }

    for (int i = 0; host->h_addr_list[i] != NULL; i ++ )
    {
        printf("%s\n",  host->h_addr_list[i]);
    }

    return 0;
}

int find_direct(char* addr)
{
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