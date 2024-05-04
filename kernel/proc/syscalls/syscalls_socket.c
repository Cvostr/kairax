#include "proc/syscalls.h"
#include "fs/vfs/vfs.h"
#include "cpu/cpu_local_x64.h"

int sys_socket(int domain, int type, int protocol)
{
    struct process* process = cpu_get_current_thread()->process;
    struct file* fsock = new_file();

    int rc = process_add_file(process, fsock);

    return rc;
}