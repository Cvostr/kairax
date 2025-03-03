#include "stdio.h"
#include <sys/socket.h> 
#include "errno.h"
#include <netinet/in.h> 
#include "syscalls.h"
#include "process.h"
#include "unistd.h"

#define SERV_PORT 22

int pty_master = 0;
int pty_slave = 0;
int client_sock = -1;

void client_send_thread()
{
    while(1)
    {
        char sym = 0;
        read(pty_master, &sym, 1);
        send(client_sock, "Welcome!\n", 9, 0);
    }
}

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

    printf("Connected client!\n");
    client_sock = clfd;
    int rc = send(clfd, "Welcome!\n", 9, 0);
    if (rc == -1)
    {
        printf("send() error: %i\n", errno);
    }

    rc = syscall_create_pty(&pty_master, &pty_slave);
    if (rc != 0)
    {
        close(clfd);
    }

    pid_t forkret = fork();
    if (forkret == 0) 
    {
        dup2(pty_slave, STDOUT_FILENO);
        dup2(pty_slave, STDIN_FILENO);
        dup2(pty_slave, STDERR_FILENO);

        int rc = execve("/rxsh.a", NULL, NULL);
        printf("exec() :%i\n", rc);
        return 22;
    }

    create_thread(client_send_thread, NULL);

    while(1) {
        ;
    }

    return 0;    
}