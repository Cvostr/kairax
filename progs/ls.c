#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/sys/syscalls.h"

int main() {

    int dirfd = syscall_open_file("/", 0, FILE_OPEN_MODE_READ_ONLY);
    struct stat dirstat;
    int rc = syscall_fdstat(dirfd, &dirstat);

    

    return 0;
}