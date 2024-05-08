#ifndef _SOCKET_H
#define _SOCKET_H

#include "kairax/in.h"
#include "fs/vfs/inode.h"

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

struct socket {
    struct inode ino;

    int state;
    int type;
    int flags;
    struct sockaddr addr;

    int	(*connect) (struct socket* sock, struct sockaddr* saddr, int sockaddr_len);
    int	(*sendmsg) (struct socket* sock, struct msghdr* m, size_t total_len);
    int	(*recvmsg) (struct socket* sock, struct msghdr* msg, size_t len, int flags, int* addr_len);
};

struct socket* new_socket();

int socket_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len);

int socket_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

#endif