#include "unistd.h"
#include "errno.h"

#define SYSINFO_HOSTNAME        5
#define SYSINFO_DOMAINNAME      6

int syscall_sysinfo(int request, void* buffer, size_t bufsize);

int syscall_sethostname(const char *name, size_t len);
int syscall_setdomainname(const char *name, size_t len);

int gethostname(char *name, size_t len)
{
    __set_errno(syscall_sysinfo(SYSINFO_HOSTNAME, name, len));
}

int sethostname(const char *name, size_t len)
{
    __set_errno(syscall_sethostname(name, len));
}

int getdomainname(char *name, size_t len)
{
    __set_errno(syscall_sysinfo(SYSINFO_DOMAINNAME, name, len));
}

int setdomainname(const char *name, size_t len)
{
    __set_errno(syscall_setdomainname(name, len));
}