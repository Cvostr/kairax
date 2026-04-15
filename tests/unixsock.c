#include "stdio.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "stdlib.h"
#include <time.h>
#include <sys/wait.h>
#include "errno.h"

#define SOCK_PATH "/tmp/testsock"
#define SOCK_INC_PATH "/tmp/testsock_incorrect"

int client();
int server();

int main(int argc, char** argv)
{
    int srv_mode = 0;
    int deletesock = 1;

    if (argc > 1 && strcmp(argv[1], "noacc") == 0)
    {
        srv_mode = 1;
    }

    for (int i = 1; i < argc; i ++)
    {
        char* arg = argv[i];

        if (strcmp(arg, "noacc") == 0)
        {
            srv_mode = 1;
        }

        if (strcmp(arg, "nodel") == 0)
        {
            deletesock = 0;
        }
        
        if (strcmp(arg, "pair") == 0)
        {
            int pre = _socketpair();
            return pre;
        }

        if (strcmp(arg, "inc") == 0)
        {
            int pre = incorrect_type();
            return pre;
        }
    }

    srand(time(NULL));
    pid_t client_pid = fork();

    if (client_pid == 0)
    {
        sleep(1);
        client();
        exit(0);
    }

    int rc = server(srv_mode, deletesock);

    int clrc = 0;
    waitpid(client_pid, &clrc, 0);

    return rc;
}

int incorrect_type()
{
    int stream_serv;
    int rc;

    if ((stream_serv = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) 
    {
        perror("socket() failed");
        return -1;
    }

    struct sockaddr_un serv_addr;
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, SOCK_INC_PATH);

    if (bind(stream_serv, (const struct sockaddr *) &serv_addr, sizeof(struct sockaddr_un)) < 0)
    {
        close(stream_serv);
        perror("server: bind()");
        return -1;
    }

    int clientsocket = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (clientsocket == -1)
	{
		perror("client: socket");
		return 1;
	}

    char tt[2];
    rc = sendto(clientsocket, tt, 2, 0, &serv_addr, sizeof(serv_addr));
    if (rc == 0)
    {
        printf("Error: should fail, but rc is ok\n");
    }

    printf("RC=%i errno=%i\n", rc, errno);

    return 0;
}

int server(int mode, int deletesock)
{
    int srv_sockfd;

    if (deletesock == 1)
    {
        unlink(SOCK_PATH);
    }

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

    if (mode == 1)
    {
        sleep(3);
        close(srv_sockfd);
        goto exit;
    }

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

exit:
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

    struct sockaddr_un peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    int rc = getpeername(clientsocket, &peer_addr, &peer_addr_len);
    if (rc != 0)
    {
        perror("getpeername()");
    }
    
    printf("Connected to server '%s'!\n", peer_addr.sun_path);

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

int _socketpair()
{
    int socks[2];
    int rc = socketpair(AF_UNIX, SOCK_STREAM, 0, socks);
    if (rc != 0)
    {
        perror("socketpair error");
        return rc;
    }

    srand(time(NULL));
    pid_t client_pid = fork();

    if (client_pid == 0)
    {
        close(socks[0]);

        char buff [40];
        char recv_buffer [40];
        for (int i = 0; i < 100; i ++)
        {
            for (int j = 0; j < sizeof(buff); j ++)
            {
                buff[j] = 32 + rand() % 20;
            }

            if (send(socks[1], buff, sizeof(buff), 0) == -1)
            {
                perror("client: send");
                return 1;
            }

            ssize_t received = recv(socks[1], recv_buffer, sizeof(recv_buffer), 0);

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
        
        exit(0);
    }

    close(socks[1]);

    ssize_t readed = 0;
    char buf[20];
    while ((readed = recv(socks[0], buf, sizeof(buf), 0)) > 0)
    {
        send(socks[0], buf, readed, 0);
    }

    int clrc = 0;
    waitpid(client_pid, &clrc, 0);
    return 0;
}