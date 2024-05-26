#include "socket.h"
#include "mem/kheap.h"
#include "string.h"
#include "kairax/errors.h"
#include "list/list.h"

list_t sock_families = {0,};
spinlock_t sock_families_lock = 0;

void register_sock_family(struct socket_family* family)
{
    acquire_spinlock(&sock_families_lock);
    list_add(&sock_families, family);
    release_spinlock(&sock_families_lock);
}

struct socket* new_socket()
{
    struct socket* result = kmalloc(sizeof(struct socket));
    memset(result, 0, sizeof(struct socket));

    result->ino.mode = INODE_FLAG_SOCKET;

    return result;
}

int socket_init(struct socket* sock, int domain, int type, int protocol)
{
    if (domain < 0 )// || domain >= 0xFFF) 
		return -ERROR_INVALID_VALUE;

    if (type < 0 || type >= SOCK_MAX)
		return -ERROR_INVALID_VALUE;

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
    return sock->ops->bind(sock, addr, addrlen);
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
    return sock->ops->setsockopt(sock, level, optname, optval, optlen);
}