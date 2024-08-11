#ifndef LOCAL_SOCKET_H
#define LOCAL_SOCKET_H

#include "kairax/types.h"
#include "ipc/socket.h"

#define LOCAL_PATH_MAX	108

struct local_socket {
    int dummy;  
    char path[LOCAL_PATH_MAX];
};

struct sockaddr_un {
    sa_family_t sun_family;
    char sun_path[LOCAL_PATH_MAX];
};

int sock_local_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);
int sock_local_listen (struct socket* sock, int backlog);
int	sock_local_accept (struct socket *sock, struct socket **newsock, struct sockaddr *addr);
ssize_t sock_local_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
int sock_local_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

int local_sock_create(struct socket* s, int type, int protocol);

struct socket_data* new_local_socket(int type);

void local_sock_init();

#endif