#include "socket.h"
#include "mem/kheap.h"
#include "string.h"
#include "kairax/errors.h"

struct socket* new_socket()
{
    struct socket* result = kmalloc(sizeof(struct socket));
    memset(result, 0, sizeof(struct socket));

    result->ino.mode = INODE_FLAG_SOCKET;

    return result;
}

int socket_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len)
{
    if (sock->connect == NULL) {
        return -ERROR_INVALID_VALUE;
    }
    
    return sock->connect(sock, saddr, sockaddr_len);
}

int socket_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    struct iovec iov = {
        .iov_base = msg,
        .iov_len = len
    };

    struct msghdr msgheader = {
		.msg_name = (void*) to,
		.msg_namelen = tolen,
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = NULL,
		.msg_controllen = 0,
		.msg_flags = 0
	};

    return sock->sendmsg(sock, &msgheader, flags);
}