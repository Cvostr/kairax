#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#include <sys/cdefs.h>
#include "types.h"
#include "stddef.h"
#include "stdint.h"

__BEGIN_DECLS

#define AF_UNSPEC	0
#define AF_LOCAL    1
#define AF_UNIX     AF_LOCAL
#define AF_INET     2
#define AF_INET6    10
#define AF_PACKET	17

#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

// дополнительные параметры 
#define SOCK_NONBLOCK   04000                
#define SOCK_CLOEXEC    02000000

// для setsockopts
#define SOL_IP		0
#define SOL_SOCKET	1

// Параметры SOL_SOCKET
#define SO_DEBUG	1
#define SO_REUSEADDR	2
#define SO_TYPE		3
#define SO_ERROR	4
#define SO_DONTROUTE	5
#define SO_BROADCAST	6
#define SO_SNDBUF	7
#define SO_RCVBUF	8
#define SO_KEEPALIVE	9
#define SO_OOBINLINE	10
#define SO_NO_CHECK	11
#define SO_PRIORITY	12
#define SO_LINGER	13
#define SO_BSDCOMPAT	14
#define SO_REUSEPORT	15
#define SO_PASSCRED	16
#define SO_PEERCRED	17
#define SO_RCVLOWAT	18
#define SO_SNDLOWAT	19
#define SO_RCVTIMEO	20
#define SO_SNDTIMEO	21
#define SO_BINDTODEVICE	25

// параметры для recv
#define MSG_DONTWAIT	0x40

#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

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

int socket(int domain, int type, int protocol) __THROW;

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) __THROW;

int listen(int sockfd, int backlog) __THROW;

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) __THROW;

int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen) __THROW;

int send(int fd, const void * buf, size_t n, int flags) __THROW;

int sendto(int sockfd, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) __THROW;

ssize_t recv(int sockfd, void* buf, size_t len, int flags) __THROW;

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) __THROW;

int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen) __THROW;

int shutdown(int sockfd, int how) __THROW;
int getpeername(int s, struct sockaddr *name, socklen_t *namelen) __THROW;
int getsockname(int s, struct sockaddr *name, socklen_t *namelen) __THROW;

__END_DECLS

#endif