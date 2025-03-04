#include "af_inet.h"
#include "ipc/socket.h"
#include "mem/kheap.h"
#include "ipv4.h"

struct socket_family ipv4_sock_family = {
    .family = AF_INET,
    .create = ipv4_sock_create
};

extern struct socket_prot_ops ipv4_stream_ops;
extern struct socket_prot_ops ipv4_dgram_ops;
struct socket_prot_ops ipv4_raw_ops = {

};

int ipv4_sock_create(struct socket* s, int type, int protocol)
{
    switch(type) {
        case SOCK_STREAM:
            s->ops = &ipv4_stream_ops;
            break;
        case SOCK_RAW:
            s->ops = &ipv4_raw_ops;
            break;
        case SOCK_DGRAM:
            s->ops = &ipv4_dgram_ops;
            break;
    }

    return s->ops->create(s);
}

void ipv4_sock_init() 
{
    register_sock_family(&ipv4_sock_family);
}