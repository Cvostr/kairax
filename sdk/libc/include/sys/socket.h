#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "stddef.h"
#include "stdint.h"

#define AF_LOCAL    1
#define AF_INET     2
#define AF_INET6    10

#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

struct iovec {
    void *iov_base;
    size_t iov_len;
};

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
};

struct msghdr {
    void* msg_name;		
    socklen_t msg_namelen;
    struct iovec* msg_iov;
    size_t msg_iovlen;	
    void* msg_control;	
    size_t msg_controllen;
    uint32_t msg_flags;
};

int socket(int domain, int type, int protocol);

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);

int send(int fd, const void * buf, size_t n, int flags);

int sendto(int sockfd, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

ssize_t recv(int sockfd, void* buf, size_t len, int flags);

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);

#ifdef __cplusplus
}
#endif

#endif