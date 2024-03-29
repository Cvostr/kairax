#include "mman.h"
#include "errno.h"
#include "stdint.h"
#include "syscalls.h"

void* mmap(void* addr, size_t length, int protection, int flags)
{
    void* result = syscall_process_map_memory(addr, length, protection, flags);
    int64_t rc = (int64_t) result;
    if (rc < 0) { 
        errno = -rc; 
        return (void*)-1;
    }
    
    return result;
}

int munmap(void* addr, size_t length)
{
    __set_errno(syscall_process_unmap_memory(addr, length));
}