#ifndef LOCAL_SOCKET_H
#define LOCAL_SOCKET_H

struct local_socket {
    int dummy;  
};

struct socket;

int local_sock_create(struct socket* s, int type, int protocol);

struct socket_data* new_local_socket(int type);

void local_sock_init();

#endif