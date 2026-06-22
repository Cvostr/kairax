#include "stdio.h"
#include <sys/socket.h> 
#include "errno.h"
#include <netinet/in.h> 
#include "syscalls.h"
#include "process.h"
#include "unistd.h"
#include "sys/ioctl.h"
#include "stdlib.h"
#include "sys/wait.h"
#include "poll.h"
#include "termios.h"
#include "string.h"

#define SERV_PORT 23

#define TELNET_IAC		0xFF
#define TELNET_WILL		0xFB
#define TELNET_DO		0xFC
#define TELNET_WONT		0xFD
#define TELNET_DONT     0xFE
#define TELNET_ECHO		0x01
#define TELNET_LINEMODE 0x22

typedef struct {
    int pty_masterfd; 
    int sockfd;
    pid_t child;
    int alive;
} telnet_client_t;

static unsigned char ECHO_DISABLE_CMD[] = { TELNET_IAC , TELNET_WILL , TELNET_ECHO };
static unsigned char SGA[] = { TELNET_IAC , TELNET_WILL , 0x3 };
static unsigned char LINEMODE_DISABLE_CMD[] = { TELNET_IAC , TELNET_DONT , TELNET_LINEMODE };

telnet_client_t *create_client(int clientsockfd)
{
    int rc;
    int pty_slavefd;

    telnet_client_t *state = malloc(sizeof(telnet_client_t));
    if (state == NULL)
    {
        return NULL;
    }

    state->sockfd = clientsockfd;
    state->alive = 1;

    rc = syscall_create_pty(&state->pty_masterfd, &pty_slavefd);
    if (rc != 0)
    {
        free(state);
        return NULL;
    }

    // Выключить эхо
    send(clientsockfd, ECHO_DISABLE_CMD, sizeof(ECHO_DISABLE_CMD), 0);
    // выключить linemode (клиент сразу отправляет нажатые кнопки)
    send(clientsockfd, LINEMODE_DISABLE_CMD, sizeof(LINEMODE_DISABLE_CMD), 0);

    // Убрать ICRNL
    struct termios tmi;
    tcgetattr(pty_slavefd, &tmi);
    // telnet всегда посылает CRLF, делаем так, чтобы не было CR и превращения CR в LF
    tmi.c_iflag &= ~ICRNL;
    tmi.c_iflag |= IGNCR;
    tcsetattr(pty_slavefd, TCSADRAIN, &tmi);

    pid_t childpid = fork();
    if (childpid == 0) 
    {
        close(clientsockfd);

        dup2(pty_slavefd, STDOUT_FILENO);
        dup2(pty_slavefd, STDIN_FILENO);
        dup2(pty_slavefd, STDERR_FILENO);
     
        close(pty_slavefd);
        close(state->pty_masterfd);

        chdir("/");
        const char* args[2];
        args[0] = "/bin/sh";
        args[1] = 0;
        int rc = execve("/bin/sh", (char *const *) args, NULL);
        printf("exec() :%i\n", rc);
        exit(22);
    }

    state->child = childpid;

    // дескрипторы на slave должны остаться только у дочернего процесса
    // чтобы не было проблем с закрытием tty
    close(pty_slavefd);

    return state;
}

void handle_data_from_client(telnet_client_t * client, const char *data, size_t len)
{
    unsigned char c;
    size_t pos = 0;
    while (pos < len)
    {
        c = (unsigned char) data[pos];

        if (c == TELNET_IAC)
        {
            pos += 3;
            continue;
        }

        write(client->pty_masterfd, &c, 1);
        pos ++;
    }
}

int main(int argc, char** argv)
{
    int rc;
    struct sockaddr_in servaddr, clientaddr;
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0); 
    
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

    printf("Server started on port %i\n", SERV_PORT);

    int clientaddr_len = sizeof(clientaddr);
    int client_sock = accept(sockfd, (struct sockaddr*) &clientaddr, &clientaddr_len);

    if (client_sock < 0) { 
        printf("server accept failed: %i\n", errno); 
        return 1;
    } 

    printf("Connected client!\n");

    telnet_client_t *client_state = create_client(client_sock);
    if (client_state == NULL)
    {
        close(client_sock);
        return 1;
    }

    struct pollfd pfds[2];
    // stdin
    pfds[0].fd = client_state->pty_masterfd;
    pfds[0].events = POLLIN | POLLHUP;
    pfds[0].revents = 0;
    // прием данных от сокета
    pfds[1].fd = client_state->sockfd;
    pfds[1].events = POLLIN | POLLHUP | POLLERR;
    pfds[1].revents = 0;

    char rcvb[30];
    ssize_t readed;
    while (client_state->alive) 
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
                readed = read(client_state->pty_masterfd, rcvb, sizeof(rcvb));
                //printf("readed %i bytes from tty\n", readed);
                if (readed == 0)
                {
                    // EOF - stdin закрылся - отправляющй процесс завершился
                    // Сообщаем пиру, что больше не будем ничего отправлять
                    shutdown(client_state->sockfd, SHUT_WR);
                    printf("stdin HUP\n");
                    client_state->alive = 0;
                    pfds[0].events = 0;
                    pfds[0].fd = -1;
                }
                else
                {
                    send(client_state->sockfd, rcvb, readed, 0);
                }
            }

            if ((revents & POLLHUP) == POLLHUP)
            {
                printf("stdin HUP\n");
                // stdin закрылся - отправляющий процесс завершился
                shutdown(client_state->sockfd, SHUT_WR);
                client_state->alive = 0;
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
                readed = recv(client_state->sockfd, rcvb, sizeof(rcvb), 0);
                //printf("received %i bytes\n", readed);
                if (readed == 0)
                {
                    printf("Client disconnected!\n");
                    client_state->alive = 0;
                    break;
                }
                else if (readed == -1)
                {
                    printf("Error in client: %s!\n", strerror(errno));
                    client_state->alive = 0;
                    break;
                }
                else
                {
                    handle_data_from_client(client_state, rcvb, readed);
                }
            }

            if ((revents & POLLHUP) == POLLHUP)
            {
                printf("Client disconnected!\n");
                client_state->alive = 0;
                break;
            }

            // Сброс полученных событий
            pfds[1].revents = 0;
        }
    }

    printf("Terminating...\n");

    // Посылаем байт в TTY чтобы разблокировать поток чтения
    char term_sym = 0x30;
    rc = ioctl(client_state->pty_masterfd, TIOCSTI, (uint64_t)&term_sym);
    if (rc != 0)
    {
        perror("ioctl(TIOCSTI)");
    }

    rc = close(client_sock);
    if (rc == -1){
        printf("Error close(client_sock): %i\n", errno);
    }

    rc = close(sockfd);
    if (rc == -1){
        printf("Error close(sockfd): %i\n", errno);
    }

    // Завершение дочерних процессов
    rc = ioctl(client_state->pty_masterfd, TIOCNOTTY, 0);
    if (rc != 0)
    {
        perror("ioctl(TIOCNOTTY)");
    }

    // Ожидание завершения дочернего процесса
    int status;
    rc = waitpid(client_state->child, &status, 0);
    printf("Process finished with status %i\n", status);

    close(client_state->pty_masterfd);

    return 0;    
}