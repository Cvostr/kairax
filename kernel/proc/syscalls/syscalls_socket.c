#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "cpu/cpu_local_x64.h"
#include "ipc/socket.h"
#include "mem/kheap.h"

struct file* make_file_from_sock(struct socket* sock)
{
    struct file* fsock = new_file();
    fsock->inode = sock->inode;
    fsock->ops = sock->inode->file_ops;
    inode_open(fsock->inode, 0);
    return fsock;
}

int sys_socket(int domain, int type, int protocol)
{
    struct process* process = cpu_get_current_thread()->process;
    
    // Запомнить доп флаги
    int cloexec = (type & SOCK_CLOEXEC) == SOCK_CLOEXEC;
    int nonblock = (type & SOCK_NONBLOCK) == SOCK_NONBLOCK;
    // Отрезать дополнительные флаги
    type = type & 0777;

    struct socket* sock = new_socket();
    if (sock == NULL)
    {
        return -ENOMEM;
    }

    int rc = socket_init(sock, domain, type, protocol);
    if (rc != 0) {
        // destroy socket
        goto exit;
    }
    
    // Создать файл из сокета
    struct file* fsock = make_file_from_sock(sock);

    // Добавить к процессу
    rc = process_add_file(process, fsock);
    if (rc < 0) 
    {
        // destroy socket
        file_close(fsock);
        goto exit;
    }

    // Если настроено неблокирующее чтение, то добавим в флаги
    if (nonblock == 1)
    {
        fsock->flags |= FILE_FLAG_NONBLOCK;
    }

    // Если передан флаг SOCK_CLOEXEC, надо взвести бит дескриптора
    if (cloexec == 1) 
    {
        process_set_cloexec(process, rc, 1);
    }

exit:
    return rc;
}

int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, addr, addrlen);

    struct file* file = process_get_file(process, sockfd);

    if (file == NULL) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) != INODE_FLAG_SOCKET) {
        rc = -ERROR_NOT_SOCKET;
        goto exit;
    }

    struct socket* sock = (struct socket*) file->inode->private_data;
    rc = socket_bind(sock, addr, addrlen);

exit:
    return rc;
}

int sys_listen(int sockfd, int backlog)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

    struct file* file = process_get_file(process, sockfd);

    if (file == NULL) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) != INODE_FLAG_SOCKET) {
        rc = -ERROR_NOT_SOCKET;
        goto exit;
    }

    struct socket* sock = (struct socket*) file->inode->private_data;
    rc = socket_listen(sock, backlog);

exit:
    return rc;
}

int sys_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

    struct file* file = process_get_file(process, sockfd);

    if (file == NULL) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) != INODE_FLAG_SOCKET) {
        rc = -ERROR_NOT_SOCKET;
        goto exit;
    }

    struct socket* sock = (struct socket*) file->inode->private_data;
    struct socket* newsock = NULL;
    rc = socket_accept(sock, &newsock, addrlen);
    if (rc == 0 && newsock != NULL) {
        // Создаем файл из сокета и добавляем
        struct file* fsock = make_file_from_sock(newsock);
        rc = process_add_file(process, fsock);
    }

exit:
    return rc;
}

int sys_setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, optval, optlen);

    struct file* file = process_get_file(process, sockfd);

    if (file == NULL) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) != INODE_FLAG_SOCKET) {
        rc = -ERROR_NOT_SOCKET;
        goto exit;
    }

    struct socket* sock = (struct socket*) file->inode->private_data;
    rc = socket_setsockopt(sock, level, optname, optval, optlen);

exit:
    return rc;
}

int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER(process, addr, addrlen);

    struct file* file = process_get_file(process, sockfd);

    if (file == NULL) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) != INODE_FLAG_SOCKET) {
        rc = -ERROR_NOT_SOCKET;
        goto exit;
    }

    struct socket* sock = (struct socket*) file->inode->private_data;
    rc = socket_connect(sock, addr, addrlen);

exit:
    return rc;
}

int sys_shutdown(int sockfd, int how)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

    struct file* file = process_get_file(process, sockfd);

    if (file == NULL) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) != INODE_FLAG_SOCKET) {
        rc = -ERROR_NOT_SOCKET;
        goto exit;
    }

    struct socket* sock = (struct socket*) file->inode->private_data;
    rc = socket_shutdown(sock, how);

exit:
    return rc;
}

int sys_getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = process_get_file(process, sockfd);

    if (file == NULL) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) != INODE_FLAG_SOCKET) {
        rc = -ERROR_NOT_SOCKET;
        goto exit;
    }

    struct socket* sock = (struct socket*) file->inode->private_data;
    rc = socket_getpeername(sock, addr, addrlen);

exit:
    return rc;
}

int sys_getsockname(int sockfd, struct sockaddr *name, socklen_t *namelen)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;
    struct file* file = process_get_file(process, sockfd);

    if (file == NULL) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) != INODE_FLAG_SOCKET) {
        rc = -ERROR_NOT_SOCKET;
        goto exit;
    }

    struct socket* sock = (struct socket*) file->inode->private_data;
    rc = socket_getsockname(sock, name, namelen);

exit:
    return rc;
}

int sys_sendto(int sockfd, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

    if (to != NULL) {
        // Если адрес задан, то проверить его
        VALIDATE_USER_POINTER(process, to, tolen);
    }

    struct file* file = process_get_file(process, sockfd);

    if (file == NULL) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) != INODE_FLAG_SOCKET) {
        rc = -ERROR_NOT_SOCKET;
        goto exit;
    }

    struct socket* sock = (struct socket*) file->inode->private_data;
    rc = socket_sendto(sock, msg, len, flags, to, tolen);

exit:
    return rc;
}

ssize_t sys_recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* from, socklen_t* addrlen)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

    if (len < 0)
		return -EINVAL;

    if (addrlen != NULL) {
        // Если выход addrlen задан, то проверить его
        VALIDATE_USER_POINTER(process, addrlen, sizeof(socklen_t));
    }

    if (from != NULL) {
        // Если адрес задан, то проверить его
        VALIDATE_USER_POINTER(process, from, sizeof(struct sockaddr));
    }

    struct file* file = process_get_file(process, sockfd);

    if (file == NULL) {
        rc = -ERROR_BAD_FD;
        goto exit;
    }

    if ((file->inode->mode & INODE_FLAG_SOCKET) != INODE_FLAG_SOCKET) {
        rc = -ERROR_NOT_SOCKET;
        goto exit;
    }

    // Если на дескрипторе настроено неблокирующее чтение - то модифицируем флаги
    if ((file->flags & FILE_FLAG_NONBLOCK) == FILE_FLAG_NONBLOCK)
    {
        flags |= MSG_DONTWAIT;
    }

    struct socket* sock = (struct socket*) file->inode->private_data;
    rc = socket_recvfrom(sock, buf, len, flags, from, addrlen);

exit:
    return rc;
}