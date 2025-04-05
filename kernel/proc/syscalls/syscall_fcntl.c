#include "proc/syscalls.h"
#include "proc/process.h"
#include "cpu/cpu_local.h"


#define F_DUPFD         0
#define F_GETFD         1
#define F_SETFD         2
#define F_GETFL         3
#define F_SETFL         4
#define F_GETLK	        5
#define F_SETLK	        6
#define F_SETLKW	    7

#define FD_CLOEXEC	    1

#define FCNTL_SETFL_MASK (FILE_OPEN_FLAG_APPEND | FILE_FLAG_NONBLOCK)

int sys_fcntl(int fd, int cmd, unsigned long long arg)
{
    struct process* process = cpu_get_current_thread()->process;

    struct file* file = process_get_file(process, fd);

    if (file == NULL) {
        return -ERROR_BAD_FD;
    }

    int cloexec;

    switch (cmd) 
    {
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
            return -1;
            

        case F_GETFD:
            cloexec = process_get_cloexec(process, fd);
            return FD_CLOEXEC * cloexec;
        case F_SETFD:
            cloexec = (arg & FD_CLOEXEC) == FD_CLOEXEC;
            process_set_cloexec(process, fd, cloexec);
            break;


        case F_GETFL:
            return file->flags;
        case F_SETFL:
            int masked = arg & FCNTL_SETFL_MASK;
            int unmask = file->flags & ~(FCNTL_SETFL_MASK);
            file->flags = unmask | masked; 
            break;

        default: {
            return -EINVAL;
        }
    }

    return 0;
}