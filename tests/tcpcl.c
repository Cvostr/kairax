#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <time.h>
#include "errno.h"

#define DEFAULT_ADDR "10.0.2.2"
#define PORT 12346
#define BUFLEN 256

#define SA struct sockaddr

void die(const char* message) 
{
    puts(message);
    printf("errno = %i\n", errno);
    exit(1);
}

int main(int argc, char** argv) 
{
    int sockfd = 0;
    int rc = 0;
    ssize_t  sent = 0;

    char addr[20];
    strcpy(addr, DEFAULT_ADDR);
    int port = PORT;

    if (argc >= 2)
    {
        strcpy(addr, argv[1]);
    }

    if (argc == 3)
    {
        port = atoi(argv[2]);
    }

    struct sockaddr_in servaddr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        die("fail socket()");
    }

    servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(addr);

    printf("Connecting to %s:%i\n", addr, port);

    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))) 
    {
        die("fail connect()");
    }

    char msg[BUFLEN];
    memset(msg, 52, BUFLEN);

    sent = send(sockfd, msg, BUFLEN, 0);

    ssize_t recvd = recv(sockfd, msg, BUFLEN, 0);

    for (int i = 0; i < recvd; i ++)
    {
        putchar(msg[i]);
    }
    fflush(stdout);

    return 0;
}