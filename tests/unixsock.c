#include "stdio.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "stdlib.h"
#include <time.h>
#include <sys/wait.h>

#define SOCK_PATH "/testsock"

int client();
int server();

int main(int argc, char** argv)
{
    srand(time(NULL));
    pid_t client_pid = fork();

    if (client_pid == 0)
    {
        sleep(1);
        client();
        exit(0);
    }

    int rc = server();

    int clrc = 0;
    waitpid(client_pid, &clrc, 0);

    return rc;
}

int server()
{
    int srv_sockfd;

    unlink(SOCK_PATH);

    if ((srv_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) 
    {
        perror("socket() failed");
        return -1;
    }

    struct sockaddr_un srv_addr;
    memset(&srv_addr, 0, sizeof(struct sockaddr_un));
    srv_addr.sun_family = AF_UNIX;
    strcpy(srv_addr.sun_path, SOCK_PATH);

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

    /*while (1) 
    {
        struct sockaddr_un client_addr;
        int client_addr_len = sizeof(struct sockaddr_un);
        int client_sock = accept(srv_sockfd, &client_addr, &client_addr_len);
        if (client_sock == -1) {
            perror("server: accept()");
            continue;
        }

        printf("Connected client!\n");
    }*/

    struct sockaddr_un client_addr;
    int client_addr_len = sizeof(struct sockaddr_un);
    int client_sock = accept(srv_sockfd, &client_addr, &client_addr_len);
    if (client_sock == -1) {
        perror("server: accept()");
        return 1;
    }

    printf("Connected client!\n");

    ssize_t readed = 0;
    char buf[20];
    while ((readed = recv(client_sock, buf, sizeof(buf), 0)) > 0)
    {
        send(client_sock, buf, readed, 0);
    }

    printf("Server closing \n");

    return 0;
}

int client()
{
    int clientsocket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (clientsocket == -1)
	{
		perror("client: socket");
		return 1;
	}

    struct sockaddr_un connect_addr;
    connect_addr.sun_family = AF_UNIX;
    strcpy(connect_addr.sun_path, SOCK_PATH);

    if (connect(clientsocket, (struct sockaddr*) &connect_addr, sizeof(connect_addr)) == -1)
	{
		perror("client: connect");
		return 1;
	}

    printf("Connected to server!\n");

    char buff [40];
    char recv_buffer [40];
    for (int i = 0; i < 100; i ++)
    {
        for (int j = 0; j < sizeof(buff); j ++)
        {
            buff[j] = 32 + rand() % 20;
        }

        if (send(clientsocket, buff, sizeof(buff), 0) == -1)
        {
            perror("client: send");
		    return 1;
        }

        ssize_t received = recv(clientsocket, recv_buffer, sizeof(recv_buffer), 0);

        if (received != 40)
        {
            printf("Error: received %i bytes\n", received);
        }

        if (memcmp(buff, recv_buffer, sizeof(buff)) != 0)
        {
            printf("Error: results missmatch\n");
            printf("orig: %s\n", buff);
            printf("recv: %s\n", recv_buffer);
        }

        printf("R %i ", i);
        fflush(NULL);
    }

    return 0;
}