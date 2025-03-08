#include <sys/socket.h>
#include "netinet/ip_icmp.h"
#include "strings.h"
#include <arpa/inet.h>
#include "stddef.h"
#include "unistd.h"
#include "stdio.h"

unsigned short checksum(void *b, int len);

int main(int argc, char** argv) 
{
    char* addr = NULL;
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

    if (addr == NULL) {
        printf("Address not specified\n");
        return 1;
    } else {
        ip4_addr = inet_addr(addr);
        if (ip4_addr == (in_addr_t) - 1) {
            printf("Incorrect address\n");
            return 1;
        }
    }

    struct icmphdr ichdr;
    bzero(&ichdr, sizeof(struct icmphdr));

    ichdr.type = ICMP_ECHO;
    ichdr.un.echo.id = getpid();
    ichdr.un.echo.sequence = 0;

    struct sockaddr_in ping_addr;
    ping_addr.sin_family = AF_INET;
	ping_addr.sin_port = htons(0);
	ping_addr.sin_addr.s_addr = ip4_addr;

    struct sockaddr_in recv_addr;
    int recv_addr_len = sizeof(struct sockaddr_in);

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        printf("socket() error\n");
        return 1;
    }

    char recv_buffer[128];
    int i = 0;
    while (i < ping_times) 
    {
        ichdr.un.echo.sequence = htons(i);
        ichdr.checksum = 0;
        ichdr.checksum = checksum(&ichdr, sizeof(ichdr));
        
        sendto(sockfd, &ichdr, sizeof(ichdr), 0, (struct sockaddr*) &ping_addr, sizeof(ping_addr));

        recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr*) &recv_addr, &recv_addr_len);

        struct icmphdr* recv_hdr = (struct icmphdr*) recv_buffer;

        if (recv_hdr->type == ICMP_ECHOREPLY && recv_hdr->un.echo.id == ichdr.un.echo.id) 
        {
            printf("Ping response from %s: icmp seq=%i\n", addr, htons(recv_hdr->un.echo.sequence));
        }

        i++;
    }
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