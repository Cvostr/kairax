#ifndef _SOCKET_H
#define _SOCKET_H

#include "kairax/in.h"
#include "fs/vfs/inode.h"

#define AF_LOCAL    1
#define AF_INET     2
#define AF_INET6    10

#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3
#define SOCK_MAX    (SOCK_RAW + 1)

struct iovec {
    void *iov_base;
    size_t iov_len;
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

#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

struct protocol {
    int type;
    int protocol;
    struct socket_ops* ops;
};

#define SOCKET_STATE_UNCONNECTED    1

struct socket_data;
struct socket_prot_ops;

struct socket {
    
    struct inode ino;

    int type;
    int protocol;

    int state;
    int flags;

    // Данные в зависимости от протокола
    struct socket_data* data;

    // Операции в зависимости от протокола
    struct socket_prot_ops* ops;
};

struct socket_prot_ops {
    int	(*connect) (struct socket* sock, struct sockaddr* saddr, int sockaddr_len);
    int (*bind) (struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);
    int (*listen) (struct socket* sock, int backlog);
    int (*recvfrom) (struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
    int (*sendto) (struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
    int (*setsockopt) (struct socket *sock, int level, int optname, const void *optval, unsigned int optlen);
    int	(*accept) (struct socket *sock, struct socket **newsock, struct sockaddr *addr);

    //int	(*sendmsg) (struct socket* sock, struct msghdr* m, int flags);
    //int	(*recvmsg) (struct socket* sock, struct msghdr* msg, int flags);
};

struct socket_family {
    int family;
    int (*create) (struct socket*, int type, int protocol);
};

void register_sock_family(struct socket_family* family);

struct socket* new_socket();

int socket_init(struct socket* sock, int domain, int type, int protocol);

int socket_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len);

int socket_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);

int socket_listen(struct socket* sock, int backlog);

int socket_accept(struct socket *sock, struct socket **newsock, struct sockaddr *addr);

int socket_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

ssize_t socket_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);

int socket_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen);

#endif