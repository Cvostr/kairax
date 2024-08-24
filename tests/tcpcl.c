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

    struct sockaddr_in servaddr;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        die("fail socket()");
    }

    servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = inet_addr("10.0.2.2");

    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))) 
    {
        die("fail connect()");
    }

    char msg[BUFLEN];
    memset(msg, 52, BUFLEN);

    sent = send(sockfd, msg, BUFLEN, 0);

    return 0;
}