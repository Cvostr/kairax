#include "../../sdk/sys/syscalls.h"
#include "elf.h"

extern int cfd;

void* link(int index, void* arg) {
    syscall_write(cfd, "w\7LINKER\n", 9);
    uint64_t* got_func_addr = (uint64_t*) arg + index; 
    return 0;
}