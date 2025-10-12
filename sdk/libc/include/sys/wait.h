#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include "types.h"

#define WNOHANG		0x00000001

#define	WEXITSTATUS(status)	(((status) & 0xff00) >> 8)
#define	WIFSTOPPED(status)	(((status) & 0xff) == 0x7f)

#define WTERMSIG(status)	((status) & 0x7f)
#define WSTOPSIG(status)	WEXITSTATUS (status)
#define WIFEXITED(status)	(WTERMSIG(status) == 0)
#define WIFSIGNALED(status)	(!WIFSTOPPED(status) && !WIFEXITED(status))

pid_t wait(int *status);

pid_t waitpid(pid_t pid, int *status, int options);

#endif