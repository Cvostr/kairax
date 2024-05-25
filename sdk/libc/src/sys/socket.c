#include "sys/socket.h"
#include "errno.h"
#include "syscalls.h"
#include "stddef.h"

int socket(int domain, int type, int protocol)
{
    __set_errno(syscall_socket(domain, type, protocol));
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    __set_errno(syscall_bind(sockfd, addr, addrlen));
}

int listen(int sockfd, int backlog)
{
    __set_errno(syscall_listen(sockfd, backlog));
}

int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    __set_errno(syscall_setsockopt(s, level, optname, optval, optlen));
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    __set_errno(syscall_accept(sockfd, addr, addrlen));
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    __set_errno(syscall_connect(sockfd, addr, addrlen));
}

int send(int sockfd, const void* buf, size_t n, int flags)
{
    return sendto(sockfd, buf, n, flags, NULL, 0);
}

int sendto(int sockfd, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    __set_errno(syscall_sendto(sockfd, msg, len, flags, to, tolen));
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags)
{
    return recvfrom(sockfd, buf, len, flags, NULL, NULL);
}

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    __set_errno(syscall_recvfrom(sockfd, buf, len, flags, src_addr, addrlen));
}