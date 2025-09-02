#include "af_packet.h"
#include "ipc/socket.h"
#include "mem/kheap.h"

extern struct socket_prot_ops packet_raw_ops;

int packet_sock_create(struct socket* s, int type, int protocol)
{
    if (type != SOCK_RAW)
    {
        return -EINVAL;
    }

    s->ops = &packet_raw_ops;

    return s->ops->create(s);
}

struct socket_family packet_sock_family = {
    .family = AF_PACKET,
    .create = packet_sock_create
};

void packet_sock_init() 
{
    register_sock_family(&packet_sock_family);
}