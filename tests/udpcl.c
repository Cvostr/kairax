#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "errno.h"
   
#define PORT     8080 
#define MAXLINE 1024 
   
// Driver code 
int main() { 
    int sockfd; 
    char buffer[MAXLINE]; 
    const char *hello = "Hello from client"; 
    struct sockaddr_in     servaddr; 
   
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
   
    memset(&servaddr, 0, sizeof(servaddr)); 
       
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = inet_addr("10.0.2.2"); 
       
    int n;
    socklen_t len = sizeof(servaddr); 
       
    sendto(sockfd, (const char *)hello, strlen(hello), 
        0, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
    printf("Hello message sent.\n"); 
           
    n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                0, (struct sockaddr *) &servaddr, 
                &len); 
    if (n < 0) {
        printf("recvfrom failed: %i\n", errno);
        return 1;
    }

    buffer[n] = '\0'; 
    printf("Server : %s\n", buffer); 
   
    close(sockfd); 
    return 0; 
}