#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include "types.h"

#define	WEXITSTATUS(status)	(((status) & 0xff00) >> 8)
#define	WIFSTOPPED(status)	(((status) & 0xff) == 0x7f)

#define WSTOPSIG(status)	WEXITSTATUS (status)

pid_t wait(int *status);

pid_t waitpid(pid_t pid, int *status, int options);

#endif