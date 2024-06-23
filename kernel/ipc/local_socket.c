#include "local_socket.h"
#include "socket.h"
#include "mem/kheap.h"
#include "kairax/errors.h"
#include "list/list.h"
#include "kairax/string.h"
#include "fs/vfs/file.h"

struct socket_family local_sock_family = {
    .family = AF_LOCAL,
    .create = local_sock_create
};

struct socket_prot_ops local_stream_ops = {
    .bind = sock_local_bind,
    .listen = sock_local_listen,
    .accept = sock_local_accept,
    .sendto = sock_local_sendto,
    .recvfrom = sock_local_recvfrom
};

struct socket_prot_ops local_dgram_ops = {
    .bind = sock_local_bind,
    .sendto = sock_local_sendto,
    .recvfrom = sock_local_recvfrom
};

list_t local_sockets = {0};

int sock_local_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen)
{
    if (addrlen < sizeof(sa_family_t)) {
        return -EINVAL;
    }

    if (addr->sa_family != AF_LOCAL) {
        return -EINVAL;
    }

    int namelen = addrlen - sizeof(sa_family_t);

    if (namelen < 1) {
        return -EINVAL;
    }

    struct sockaddr_un* addr_un = (struct sockaddr_un*) addr;
    struct local_socket* lsock = sock->data;

    printk("Opening file %s\n", addr_un->sun_path);

    if (addr_un->sun_path[0] == 0 && (namelen - 1) > 0) {
        // без ФС
        //strcpy(lsock->path, addr_un->sun_path + 1);
        //list_add(&local_sockets, lsock); 
    } else {
        // Открыть файл в ядре
        struct file* file = file_open(NULL, addr_un->sun_path, FILE_OPEN_FLAG_CREATE, 0);
        if (file == NULL) {
            return -1;
        }
    }
    
    return 0;
}

int sock_local_listen(struct socket* sock, int backlog)
{
    return 0;
}

int	sock_local_accept (struct socket *sock, struct socket **newsock, struct sockaddr *addr)
{
    return 0;
}

int sock_local_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    return 0;
}

int sock_local_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    return 0;
}

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