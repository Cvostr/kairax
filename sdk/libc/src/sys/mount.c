#include "sys/mount.h"
#include "errno.h"
#include "syscalls.h"

int mount(const char* device, const char* mount_dir, const char* fs, unsigned long flags)
{
    __set_errno(syscall_mount(device, mount_dir, fs, flags));
}

int unmount(const char *mount_dir, int flags)
{
    __set_errno(syscall_unmount(mount_dir, flags));
}