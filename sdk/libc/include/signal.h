#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "sys/cdefs.h"
#include "sys/types.h"

#define	SIG_BLOCK     0		 
#define	SIG_UNBLOCK   1		 
#define	SIG_SETMASK   2

#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGSEGV		11
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGUNUSED	31

__BEGIN_DECLS

typedef unsigned long sigset_t;

int raise(int sig) __THROW;
int kill(pid_t pid, int sig) __THROW;

int sigaddset(sigset_t *set, int sig) __THROW;
int sigdelset(sigset_t *set, int sig) __THROW;
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) __THROW;
int sigpending(sigset_t *set) __THROW;

__END_DECLS

#endif