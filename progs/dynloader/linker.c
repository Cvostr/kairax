#include "../../sdk/sys/syscalls.h"

extern int cfd;

void* link(int index, void* arg) {
    syscall_write(cfd, "w\7LINKER\n", 9);
    return 0;
}