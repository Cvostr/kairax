#ifndef LOCAL_SOCKET_H
#define LOCAL_SOCKET_H

#include "kairax/types.h"
#include "ipc/socket.h"

#define LOCAL_PATH_MAX	108

struct local_socket {
    int dummy;  
};


int sock_local_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);
int sock_local_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
int sock_local_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);

int local_sock_create(struct socket* s, int type, int protocol);

struct socket_data* new_local_socket(int type);

void local_sock_init();

#endif