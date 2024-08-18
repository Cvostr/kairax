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

#define PORT 22
#define BUFLEN 512

int quiet = 0;
int msgs = 0;
int limit = 0;

void die(const char* message) {
    puts(message);
    exit(1);
}

int main(int argc, char** argv) 
{

	for (int i = 0; i < argc; i ++) {
		if (strcmp(argv[i], "q") == 0) {
			quiet = 1;
		}
		if (strcmp(argv[i], "l") == 0) {
			limit = 1;
		}
	}

    int sockfd = 0;
    ssize_t recv_len;
    char buf[BUFLEN];
    memset(buf, 0, BUFLEN);
    struct sockaddr_in servaddr, claddr;
    int claddr_len = sizeof(claddr);

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Fail");
    }


    servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		die("bind");
	}

    while(1)
	{
		if (quiet == 0) {
			printf("Waiting for data...");
			fflush(stdout);
		}
		
        memset(buf, 0, BUFLEN);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(sockfd, buf, BUFLEN, 0, (struct sockaddr *) &claddr, &claddr_len)) == -1)
		{
			die("recvfrom()");
		}
		
		msgs++;

		if (quiet == 0) {

			//print details of the client/peer and the data received
			printf("Received packet from %s:%d\n", inet_ntoa(claddr.sin_addr), ntohs(claddr.sin_port));
			printf("Data: '%s'\n" , buf);
		}
		
		//now reply the client with the same data
		if (sendto(sockfd, buf, recv_len, 0, (struct sockaddr*) &claddr, claddr_len) == -1)
		{
			die("sendto()");
		}

		if (limit == 1 && msgs == 2) {
			close(sockfd);
			return 0;
		}
	}

	return 0;
}