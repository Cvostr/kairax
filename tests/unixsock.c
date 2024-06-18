#include "stdio.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char** argv)
{
    int srv_sockfd;

    if ((srv_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) 
    {
        perror("socket() failed");
        return -1;
    }

    struct sockaddr_un srv_addr;
    memset(&srv_addr, 0, sizeof(struct sockaddr_un));
    srv_addr.sun_family = AF_UNIX;
    strcpy(srv_addr.sun_path, "#testserver");
    srv_addr.sun_path[0] = 0;

    if (bind(srv_sockfd, (const struct sockaddr *) &srv_addr, sizeof(struct sockaddr_un)) < 0)
    {
        close(srv_sockfd);
        perror("server: bind");
        return -1;
    }
}