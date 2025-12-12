#include "sys/mman.h"
#include "fcntl.h"
#include "errno.h"
#include "stdint.h"
#include "syscalls.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

static const char shm_default_dir[] = "/dev/shm/";

void* mmap(void* addr, size_t length, int protection, int flags, int fd, off_t offset)
{
    void* result = syscall_map_memory(addr, length, protection, flags, fd, offset);
    int64_t rc = (int64_t) result;
    if (rc < 0) { 
        errno = -rc; 
        return MAP_FAILED;
    }
    
    return result;
}

int mprotect (void *addr, size_t length, int protection)
{
    __set_errno(syscall_protect_memory(addr, length, protection));
}

int munmap(void* addr, size_t length)
{
    __set_errno(syscall_process_unmap_memory(addr, length));
}

int shm_open(const char *name, int oflag, mode_t mode)
{
    // пропускаем все /
    while (*name == '/')
        name ++;

    if (*name == '\0')
    {
        errno = EINVAL;
        return -1;
    }

    size_t namelen = strlen(name);
    char* actual_path = malloc(strlen(shm_default_dir) + namelen + 1);
    if (actual_path == 0)
    {
        errno = ENOMEM;
        return -1;
    }

    // сформируем итоговый путь
    strcpy(actual_path, shm_default_dir);
    strcat(actual_path, name);

    int fd = open(actual_path, oflag, mode);
    if (fd != -1)
    {
        // Должны взвести cloexec
        int flags = fcntl(fd, F_GETFD, 0);
        flags |= FD_CLOEXEC;
        flags = fcntl(fd, F_SETFD, flags);

        // проверить результат
        if (flags == -1)
        {
            close(fd);
            fd = -1;
        }
    }

    free(actual_path);

    return fd;
}

int shm_unlink(const char *name)
{   
        // пропускаем все /
    while (*name == '/')
        name ++;

    if (*name == '\0')
    {
        errno = EINVAL;
        return -1;
    }

    size_t namelen = strlen(name);
    char* actual_path = malloc(strlen(shm_default_dir) + namelen + 1);
    if (actual_path == 0)
    {
        errno = ENOMEM;
        return -1;
    }

    // сформируем итоговый путь
    strcpy(actual_path, shm_default_dir);
    strcat(actual_path, name);

    return unlink(actual_path);
}