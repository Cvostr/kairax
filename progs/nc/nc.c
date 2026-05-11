#include "errno.h"
#include "string.h"
#include "unistd.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "getopt.h"
#include "netdb.h"
#include <poll.h>

#include <sys/socket.h>

#define READ_CHUNK_SIZE (8 * 1024)

int is_listen = 0;
int udp = 0;
int verbose = 0;
int zeroio = 0;
int noresolve = 0;

struct addrinfo *getaddr(char *dest, char *port)
{
    struct addrinfo *result = NULL;
    struct addrinfo hint = {0};

    // TODO: добавить управление аргументами
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = (udp == 1) ? SOCK_DGRAM : SOCK_STREAM;

    if (noresolve == 1)
    {
        hint.ai_flags |= AI_NUMERICHOST;
    }

    int rc = getaddrinfo(dest, port, &hint, &result);
    if (rc != 0)
    {
        printf("nc: %s:%s: %s\n", dest, port, gai_strerror(rc));
        return NULL;
    }

    return result;
}

int process(char *dest, char *port)
{
    // Пробуем DNS
    struct addrinfo *addrinfo = getaddr(dest, port);
    if (addrinfo == NULL)
    {
        return 1;
    }

    struct sockaddr* addr = addrinfo->ai_addr;
    int sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (sockfd == -1)
    {
        return 1;
    }

    int rc = connect(sockfd, addr, addrinfo->ai_addrlen);
    if (rc == -1)
    {
        if (verbose == 1)
            printf("nc: connect to %s port %s failed: %s\n", dest, port, strerror(errno));
        return 1;
    }

    freeaddrinfo(addrinfo);

    if (verbose == 1)
        printf("Connection to %s port %s succeeded!\n", dest, port);

    if (zeroio == 0)
    {
        char *inread_buffer = malloc(READ_CHUNK_SIZE);

        ssize_t readed;
        struct pollfd pfds[2];
        // stdin
        pfds[0].fd = STDIN_FILENO;
        pfds[0].events = POLLIN | POLLHUP;
        pfds[0].revents = 0;
        // прием данных от сокета
        pfds[1].fd = sockfd;
        pfds[1].events = POLLIN | POLLHUP | POLLERR;
        pfds[1].revents = 0;

        int running = 1;
        while (running)
        {
            int revents;
            int events = poll(pfds, 2, -1);
            if (events == 0)
                continue;
            
            if (pfds[0].revents != 0)
            {
                revents = pfds[0].revents;
                
                if ((revents & POLLIN) == POLLIN)
                {
                    readed = read(STDIN_FILENO, inread_buffer, READ_CHUNK_SIZE);
                    if (readed == 0)
                    {
                        // EOF - stdin закрылся - отправляющй процесс завершился
                        // Сообщаем пиру, что больше не будем ничего отправлять
                        shutdown(sockfd, SHUT_WR);
                        //printf("stdin EOF\n");
                        pfds[0].events = 0;
                        pfds[0].fd = -1;
                    }
                    else
                    {
                        send(sockfd, inread_buffer, readed, 0);
                    }
                }

                if ((revents & POLLHUP) == POLLHUP)
                {
                    //printf("stdin HUP\n");
                    // stdin закрылся - отправляющий процесс завершился
                    shutdown(sockfd, SHUT_WR);
                    pfds[0].events = 0;
                    pfds[0].fd = -1;
                }

                // Сброс полученных событий
                pfds[0].revents = 0;
            }

            if (pfds[1].revents != 0)
            {
                revents = pfds[1].revents;
                if ((revents & POLLIN) == POLLIN)
                {
                    readed = recv(sockfd, inread_buffer, READ_CHUNK_SIZE, 0);
                    //printf("received %i bytes\n", readed);
                    if (readed == 0)
                    {
                        // сокет закрыл соединение
                        running = 0;
                    }
                    else if (readed == -1)
                    {
                        if (verbose == 1)
                            printf("nc: read: %s\n", strerror(errno));
                        break;
                    }
                    else
                    {
                        write(STDOUT_FILENO, inread_buffer, readed);
                    }
                }

                if ((revents & POLLHUP) == POLLHUP)
                {
                    //printf("socket HUP\n");
                    running = 0;
                }

                // Сброс полученных событий
                pfds[1].revents = 0;
            }
        }

        free(inread_buffer);
    }

    close(sockfd);

    return 0;
}

int main(int argc, char** argv) 
{
    int rc;
    int opt;

    char *destination = NULL;
    char *strport = NULL;
    uint64_t port;

    while ((opt = getopt(argc, argv, "luvzn")) != -1) {
        switch (opt) {
            case 'l': 
                is_listen = 1;
                break;
            case 'u':
                udp = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'z':
                zeroio = 1;
                break;
            case 'n':
                noresolve = 1;
                break;
        }
    }

    if (optind == argc)
    {
        printf("nc: missing destination address\n");
        return 1;
    }

    if (optind + 1 == argc)
    {
        printf("nc: missing port number\n");
        return 1;
    }

    for (; optind < argc; optind++) 
    {
        if (destination == NULL)
        {
            destination = argv[optind];
        }
        else 
        {
            char *endptr;
            strport = argv[optind];
            port = strtol(strport, &endptr, 10);

            if (*endptr == '\0')
            {
                if (port == 0)
                {
                    printf("nc: port number too small: %s", strport);
                    return 1;
                }

                if (port > 65535)
                {
                    printf("nc: port number too small: %s", strport);
                    return 1;
                }
            }
    
            rc = process(destination, strport); 
            if (rc != 0)
                return rc;
        }
    }
}