#include "socket.h"
#include "mem/kheap.h"
#include "string.h"
#include "kairax/errors.h"
#include "list/list.h"

list_t sock_families = {0,};
spinlock_t sock_families_lock = 0;

struct file_operations socket_fops = {
    .read = socket_read,
    .write = socket_write,
    .ioctl = socket_ioctl,
    .poll = socket_poll,
    .close = socket_close
};

void register_sock_family(struct socket_family* family)
{
    acquire_spinlock(&sock_families_lock);
    list_add(&sock_families, family);
    release_spinlock(&sock_families_lock);
}

struct socket* new_socket()
{
    struct socket* result = kmalloc(sizeof(struct socket));
    if (result == NULL)
    {
        return NULL;
    }

    memset(result, 0, sizeof(struct socket));

    result->state = SOCKET_STATE_UNCONNECTED;

    result->ino.mode = INODE_FLAG_SOCKET;
    result->ino.file_ops = &socket_fops;

    return result;
}

int socket_init(struct socket* sock, int domain, int type, int protocol)
{
    if (domain < 0 )// || domain >= 0xFFF) 
		return -ERROR_INVALID_VALUE;

    if (type < 0 || type >= SOCK_MAX)
		return -ERROR_INVALID_VALUE;

    sock->domain = domain;
    sock->type = type;
    sock->protocol = protocol;

    // Найти объект семейства по номеру
    acquire_spinlock(&sock_families_lock);
    struct list_node* current = sock_families.head;
    struct socket_family* fam = NULL;
    struct socket_family* chosen = NULL;

    while (current != NULL) {
        
        fam = (struct socket_family*) current->element;

        if (fam->family == domain) {
            chosen = fam;
        }

        // Переход на следующий элемент
        current = current->next;
    }
    release_spinlock(&sock_families_lock);

    if (chosen == NULL) {
        return -ERROR_INVALID_VALUE;
    }

    // Создать сокет указанного семейства
    int rc = chosen->create(sock, type, protocol);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

int socket_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len)
{
    if (sock->ops->connect == NULL) {
        return -ERROR_INVALID_VALUE;
    }
    
    return sock->ops->connect(sock, saddr, sockaddr_len);
}

int socket_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen)
{
    if (sock->ops->bind == NULL) {
        return -ERROR_INVALID_VALUE;
    }

    return sock->ops->bind(sock, addr, addrlen);
}

int socket_listen(struct socket* sock, int backlog)
{
    if (sock->ops->listen == NULL) {
        return -ERROR_INVALID_VALUE;
    }

    return sock->ops->listen(sock, backlog);
}

int socket_accept(struct socket *sock, struct socket **newsock, struct sockaddr *addr)
{
    if (sock->ops->accept == NULL) {
        return -ERROR_INVALID_VALUE;
    }

    return sock->ops->accept(sock, newsock, addr);
}

int socket_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    return sock->ops->sendto(sock, msg, len, flags, to, tolen);
}

ssize_t socket_recvfrom(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
{
    return sock->ops->recvfrom(sock, buf, len, flags, src_addr, addrlen);
}

int socket_setsockopt(struct socket* sock, int level, int optname, const void *optval, unsigned int optlen)
{
    if (sock->ops->setsockopt == NULL) {
        return -ERROR_INVALID_VALUE;
    }
    return sock->ops->setsockopt(sock, level, optname, optval, optlen);
}

int socket_shutdown(struct socket* sock, int how)
{
    if (sock->ops->shutdown == NULL) {
        return -ERROR_INVALID_VALUE;
    }

    return sock->ops->shutdown(sock, how);
}

int socket_close(struct inode *inode, struct file *file)
{
    struct socket* sock = (struct socket*) inode;
    return sock->ops->close(sock);
}

int socket_getpeername(struct socket *sock, struct sockaddr *addr, socklen_t *addrlen)
{
    if (sock->ops->getpeername == NULL) {
        return -ERROR_INVALID_VALUE;
    }

    return sock->ops->getpeername(sock, addr, addrlen);
}

int socket_getsockname(struct socket *sock, struct sockaddr *name, socklen_t *namelen)
{
    if (sock->ops->getsockname == NULL) {
        return -ERROR_INVALID_VALUE;
    }

    return sock->ops->getsockname(sock, name, namelen);
}

ssize_t socket_read(struct file* file, char* buffer, size_t count, loff_t offset)
{
    struct socket* sock = (struct socket*) file->inode;
    return socket_recvfrom(sock, buffer, count, 0, NULL, 0);
}

ssize_t socket_write(struct file* file, const char* buffer, size_t count, loff_t offset)
{
    struct socket* sock = (struct socket*) file->inode;
    return socket_sendto(sock, buffer, count, 0, NULL, 0);
}

int socket_ioctl(struct file* file, uint64_t request, uint64_t arg)
{
    struct socket* sock = (struct socket*) file->inode;

    if (sock->ops->ioctl == NULL) {
        return -ERROR_INVALID_VALUE;
    }

    return sock->ops->ioctl(sock, request, arg);
}

short socket_poll(struct file *file, struct poll_ctl *pctl)
{
    struct socket* sock = (struct socket*) file->inode;

    if (sock->ops->poll == NULL) {
        return POLLNVAL;
    }

    return sock->ops->poll(sock, file, pctl);
}