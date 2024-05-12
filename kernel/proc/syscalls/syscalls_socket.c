#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "cpu/cpu_local_x64.h"
#include "ipc/socket.h"

int sys_socket(int domain, int type, int protocol)
{
    struct process* process = cpu_get_current_thread()->process;
    struct file* fsock = new_file();

    struct socket* sock = new_socket();

    fsock->inode = (struct inode*) sock;
    inode_open(fsock->inode, 0);

    int rc = socket_init(sock, domain, type, protocol);
    if (rc != 0) {
        // destroy socket
        goto exit;
    }
    
    rc = process_add_file(process, fsock);
    if (rc != 0) {
        // destroy socket
        goto exit;
    }

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

    rc = socket_connect((struct socket*) file->inode, addr, addrlen);

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

    rc = socket_sendto((struct socket*) file->inode, msg, len, flags, to, tolen);

exit:
    return rc;
}

ssize_t sys_recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* from, socklen_t* addrlen)
{
    int rc = -1;
    struct process* process = cpu_get_current_thread()->process;

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

    rc = socket_recvfrom((struct socket*) file->inode, buf, len, flags, from, addrlen);

exit:
    return rc;
}