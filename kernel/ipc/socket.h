#ifndef _SOCKET_H
#define _SOCKET_H

#include "kairax/in.h"
#include "fs/vfs/inode.h"
#include "fs/vfs/file.h"

#define AF_LOCAL    1
#define AF_INET     2
#define AF_INET6    10
#define AF_PACKET	17

#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3
#define SOCK_MAX    (SOCK_RAW + 1)

// дополнительные параметры 
#define SOCK_NONBLOCK   04000                
#define SOCK_CLOEXEC    02000000

// параметры для recv
#define MSG_DONTWAIT	0x40

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
#define SOCKET_STATE_LISTEN         2
#define SOCKET_STATE_CONNECTING     3
#define SOCKET_STATE_CONNECTED      4

struct socket_data;
struct socket_prot_ops;

struct socket {
    
    struct inode ino;

    int domain;
    int type;
    int protocol;

    // Состояние в системе
    int state;
    // Флаги (пока не используется)
    int flags;

    // Данные в зависимости от протокола
    struct socket_data* data;

    // Операции в зависимости от протокола
    struct socket_prot_ops* ops;
};

#define SOL_IP		0
#define SOL_SOCKET	1

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

#define IP_TOS		1
#define IP_TTL		2
#define IP_HDRINCL	3

#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

struct socket_prot_ops {
    int (*create) (struct socket *sock);
    int	(*connect) (struct socket *sock, struct sockaddr* saddr, int sockaddr_len);
    int (*bind) (struct socket *sock, const struct sockaddr *addr, socklen_t addrlen);
    int (*listen) (struct socket *sock, int backlog);
    ssize_t (*recvfrom) (struct socket* sock, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    int (*sendto) (struct socket *sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
    int (*setsockopt) (struct socket *sock, int level, int optname, const void *optval, unsigned int optlen);
    int	(*accept) (struct socket *sock, struct socket **newsock, struct sockaddr *addr);
    int	(*shutdown) (struct socket *sock, int how);
    int	(*close) (struct socket *sock);
    int (*getpeername) (struct socket *sock, struct sockaddr *addr, socklen_t *addrlen);


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
int socket_shutdown(struct socket* sock, int how);
int socket_getpeername(struct socket *sock, struct sockaddr *addr, socklen_t *addrlen);

int socket_close(struct inode *inode, struct file *file);
ssize_t socket_read(struct file* file, char* buffer, size_t count, loff_t offset);
ssize_t socket_write(struct file* file, const char* buffer, size_t count, loff_t offset);

#endif