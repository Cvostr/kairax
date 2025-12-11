#include "sys/mman.h"
#include "errno.h"
#include "stdint.h"
#include "syscalls.h"

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