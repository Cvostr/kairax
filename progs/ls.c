#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/sys/syscalls.h"

int main() {

    int dirfd = syscall_open_file("/", 0, FILE_OPEN_MODE_READ_ONLY);
    struct stat dirstat;
    int rc = syscall_fdstat(dirfd, &dirstat);

    struct dirent dr;
    while(readdir(dirfd, &dr) == 1) {
        printf("TYPE %s, NAME %s\n", (dr.type == DT_REG) ? "FILE" : "DIR", dr.name);
    }    

    return 0;
}