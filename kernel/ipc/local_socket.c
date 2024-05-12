#include "local_socket.h"
#include "socket.h"
#include "mem/kheap.h"

struct socket_family local_sock_family = {
    .family = AF_LOCAL,
    .create = local_sock_create
};

struct socket_prot_ops local_stream_ops = {

};

struct socket_prot_ops local_dgram_ops = {

};

int local_sock_create(struct socket* s, int type, int protocol) 
{
    switch(type) {
        case SOCK_STREAM:
            s->ops = &local_stream_ops;
            break;
        case SOCK_RAW:
        case SOCK_DGRAM:
            s->ops = &local_dgram_ops;
            break;
    }

    s->data = new_local_socket(type);
    return 0;
}

struct socket_data* new_local_socket(int type)
{
    return kmalloc(sizeof(struct local_socket));
}

void local_sock_init() 
{
    register_sock_family(&local_sock_family);
}