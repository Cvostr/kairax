#include "dirent.h"
#include "syscalls.h"
#include "errno.h"
#include "unistd.h"
#include <fcntl.h>
#include "stdlib.h"

#define PAGE_SIZE 4096UL

DIR *opendir(const char *name)
{
    DIR* dir = NULL;
    int fd = open(name, O_RDONLY | O_DIRECTORY, 0);

    if (fd >= 0 ) {
        dir = malloc(PAGE_SIZE);
        
        if (!dir) {
            goto exit;
        }

        dir->fd = fd;
    }

exit:
    return dir;
}

DIR *fdopendir(int fd)
{
    DIR* dir = NULL;

    if (fd >= 0 ) {
        dir = malloc(PAGE_SIZE);

        if (!dir) {
            goto exit;
        }

        dir->fd = fd;
    }

exit:
    return dir;
}

int closedir(DIR *dir)
{
    int rc = close(dir->fd);
    free(dir);
    return rc;
}

struct dirent *readdir(DIR *dir)
{
    int rc = syscall_readdir(dir->fd, &dir->drent);

    if (rc == 0) {
        return NULL;
    }

    if (rc < 0) { 
        errno = -rc;
        return NULL;
    }

    return &dir->drent;
}