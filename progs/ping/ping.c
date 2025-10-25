#include <sys/socket.h>
#include "netinet/ip_icmp.h"
#include "string.h"
#include "strings.h"
#include <arpa/inet.h>
#include "stddef.h"
#include "unistd.h"
#include "stdio.h"
#include "errno.h"
#include "netdb.h"
#include <stdlib.h>
#include "signal.h"

unsigned short checksum(void *b, int len);

int transmitted = 0;
int received = 0;

void print_stats()
{
    printf("%i packets transmitted, %i received\n", transmitted, received);
}

void inthandler(int arg)
{
    print_stats();
    exit(0);
}

int main(int argc, char** argv) 
{
    char* addr = NULL;
    char* real_addr = NULL;
    in_addr_t ip4_addr = 0;
    int ping_times = 4;

    for (int i = 1; i < argc; i ++)
    {
        char* arg = argv[i];
        if (arg[0] == '-') 
        {
            // параметр
        } else {
            addr = arg;
        }
    }

    int sockfamily = AF_INET;
    socklen_t ping_addr_size = 0;
    struct sockaddr* ping_addr = NULL;

    if (addr == NULL) 
    {
        printf("ping: Address not specified\n");
        return 1;
    } 
    else 
    {
        ip4_addr = inet_addr(addr);
        if (ip4_addr != (in_addr_t) - 1) 
        {
            // Корректный адрес 
            sockfamily = AF_INET;
            ping_addr_size = sizeof(struct sockaddr_in);
            ping_addr = malloc(sizeof(struct sockaddr_in));   
            struct sockaddr_in *paddr = ping_addr;
            paddr->sin_family = AF_INET;
            paddr->sin_port = htons(0);
            paddr->sin_addr.s_addr = ip4_addr;

            real_addr = strdup(addr);
        }
        else
        {
            // Пробуем DNS
            struct addrinfo *result = NULL;
            struct addrinfo hint = {0};
            hint.ai_family = AF_INET;
            int rc = getaddrinfo(addr, NULL, &hint, &result);
            if (rc != 0)
            {
                printf("ping: %s: %s\n", addr, gai_strerror(rc));
                return 1;
            }

            sockfamily = result->ai_family;
            ping_addr_size = result->ai_addrlen;
            ping_addr = malloc(ping_addr_size);   
            memcpy(ping_addr, result->ai_addr, ping_addr_size);

            freeaddrinfo(result);

            char* netaddr = NULL;
            switch (ping_addr->sa_family)
            {
                case AF_INET:
                    netaddr = malloc(INET_ADDRSTRLEN);
                    struct sockaddr_in *paddr = ping_addr;
                    inet_ntop(ping_addr->sa_family, &paddr->sin_addr, netaddr, INET_ADDRSTRLEN);
                    break;
                default:
                    printf("Not implemented for af %i\n", ping_addr->sa_family);
                    return 1;
            }

            real_addr = malloc(200);
            sprintf(real_addr, "%s (%s)", result->ai_canonname, netaddr);
        }
    }

    struct icmphdr ichdr;
    bzero(&ichdr, sizeof(struct icmphdr));

    ichdr.type = ICMP_ECHO;
    ichdr.un.echo.id = getpid();
    ichdr.un.echo.sequence = 0;

    struct sockaddr_in recv_addr;
    int recv_addr_len = sizeof(struct sockaddr_in);

    printf("PING %s\n", real_addr);

    signal(SIGINT, inthandler);

    int sockfd = socket(sockfamily, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        printf("ping: socket() error: %s\n", strerror(errno));
        return 1;
    }

    struct icmphdr* recv_hdr;
    char recv_buffer[128];
    int i = 0;
    int rc = 0;
    while (i < ping_times) 
    {
        usleep(1000000);

        i++;

        ichdr.un.echo.sequence = htons(i);
        ichdr.checksum = 0;
        ichdr.checksum = checksum(&ichdr, sizeof(ichdr));
        
        rc = sendto(sockfd, &ichdr, sizeof(ichdr), 0, ping_addr, ping_addr_size);
        transmitted ++;
        if (rc == -1)
        {
            printf("Error sending ping packet to %s: %i\n", addr, errno);
            continue;
        }

        while (1)
        {
            recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr*) &recv_addr, &recv_addr_len);

            recv_hdr = (struct icmphdr*) recv_buffer;

            if (recv_hdr->type == ICMP_ECHOREPLY && recv_hdr->un.echo.id == ichdr.un.echo.id) 
            {
                received++;
                printf("Ping response from %s: icmp seq=%i\n", real_addr, htons(recv_hdr->un.echo.sequence));
                break;
            }
        }
    }

    print_stats();
}

unsigned short checksum(void *b, int len) 
{
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}