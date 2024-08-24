#include "stdio.h"
#include <sys/socket.h> 
#include "errno.h"
#include <netinet/in.h> 

#define SERV_PORT 22

int main(int argc, char** argv)
{
    struct sockaddr_in servaddr, clientaddr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    
    if (sockfd == -1) {
        printf("Error creating server socket: %i\n", errno);
        return 1;
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(SERV_PORT); 

    if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed: %i\n", errno); 
        return 1;
    } 

    if ((listen(sockfd, 5)) != 0) { 
        printf("socket listen failed: %i\n", errno); 
        return 1;
    } 

    int clientaddr_len = sizeof(clientaddr);
    int clfd = accept(sockfd, &clientaddr, &clientaddr_len);

    if (clfd < 0) { 
        printf("server accept failed: %i\n", errno); 
        return 1;
    } 

    return 0;    
}