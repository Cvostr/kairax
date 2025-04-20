#ifndef LOCAL_SOCKET_H
#define LOCAL_SOCKET_H

#include "kairax/types.h"
#include "ipc/socket.h"
#include "list/list.h"
#include "sync/spinlock.h"
#include "proc/blocker.h"

#define LOCAL_PATH_MAX	108

struct local_socket {
    char path[LOCAL_PATH_MAX];
    struct inode* bound_inode;

    uint32_t backlog_sz;  
    list_t backlog;
    spinlock_t backlog_lock;

    list_t rx_queue;
    size_t rx_available;
    spinlock_t rx_queue_lock;

    struct blocker backlog_blk;
    struct blocker rx_blk;
    struct blocker connect_blk;

    // Для сокета клиента - сокет на стороне сервера
    // Для сервера - сокет клиента
    struct socket* peer;
    // Признак того, что соединение со стороны сервера было принято
    int connection_accepted;
    int peer_disconnected;
};

struct sockaddr_un {
    sa_family_t sun_family;
    char sun_path[LOCAL_PATH_MAX];
};

struct local_sock_bucket {
    size_t size;
    off_t offset;
    // передаваемые данные будут сразу тут
    unsigned char data[];
};

int socket_local_append_to_backlog(struct socket* server_sock, struct socket* sock);

int sock_local_bind(struct socket* sock, const struct sockaddr *addr, socklen_t addrlen);
int sock_local_listen (struct socket* sock, int backlog);
int	sock_local_connect(struct socket* sock, struct sockaddr* saddr, int sockaddr_len);
int	sock_local_accept (struct socket *sock, struct socket **newsock, struct sockaddr *addr);
int sock_local_close(struct socket* sock);
int sock_local_sendto(struct socket* sock, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
ssize_t sock_local_recvfrom_stream(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
ssize_t sock_local_recvfrom_dgram(struct socket* sock, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);

int local_sock_create(struct socket* s, int type, int protocol);

struct socket_data* new_local_socket(int type);

void local_sock_init();

#endif