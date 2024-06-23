#include "stdio.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char** argv)
{
    int srv_sockfd;

    if ((srv_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) 
    {
        perror("socket() failed");
        return -1;
    }

    struct sockaddr_un srv_addr;
    memset(&srv_addr, 0, sizeof(struct sockaddr_un));
    srv_addr.sun_family = AF_UNIX;
    strcpy(srv_addr.sun_path, "/testsock");
    //srv_addr.sun_path[0] = 0;

    if (bind(srv_sockfd, (const struct sockaddr *) &srv_addr, sizeof(struct sockaddr_un)) < 0)
    {
        close(srv_sockfd);
        perror("server: bind()");
        return -1;
    }

    if (listen(srv_sockfd, 5) != 0 )
	{
        close(srv_sockfd);
		perror("server: listen()");
        return -1;
	}

    while (1) {

        struct sockaddr_un client_addr;
        int client_addr_len = sizeof(struct sockaddr_un);
        int client_sock = accept(srv_sockfd, &client_addr, &client_addr_len);
        if (client_sock == -1) {
            perror("server: accept()");
        }
    }
}