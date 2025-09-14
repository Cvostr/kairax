#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "sys/cdefs.h"
#include "sys/types.h"

#define	SIG_BLOCK     0		 
#define	SIG_UNBLOCK   1		 
#define	SIG_SETMASK   2

#define NSIG         32

#define SIGHUP          1
#define SIGINT          2
#define SIGQUIT         3
#define SIGILL          4
#define SIGTRAP         5
#define SIGABRT         6
#define SIGIOT		    6
#define SIGBUS          7
#define SIGFPE          8
#define SIGKILL         9
#define SIGUSR1         10
#define SIGSEGV         11
#define SIGUSR2         12
#define SIGPIPE         13
#define SIGALRM         14
#define SIGTERM         15
#define SIGSTKFLT       16
#define SIGCHLD         17
#define SIGCONT         18
#define SIGSTOP         19
#define SIGTSTP         20
#define SIGTTIN         21
#define SIGTTOU         22
#define SIGURG          23
#define SIGXCPU         24
#define SIGXFSZ         25
#define SIGVTALRM       26
#define SIGPROF         27
#define SIGWINCH        28

#define SA_SIGINFO	    0x00000004

__BEGIN_DECLS

typedef unsigned long sigset_t;

union __sigval
{
    int __sival_int;
    void *__sival_ptr;
};

typedef union __sigval __sigval_t;
typedef __sigval_t sigval_t;

typedef struct {
    int      si_signo; 
    int      si_errno; 
    int      si_code;  
    pid_t    si_pid;   
    uid_t    si_uid;   
    int      si_status;
    clock_t  si_utime; 
    clock_t  si_stime; 
    sigval_t si_value; 
    int      si_int;   
    void *   si_ptr;   
    void *   si_addr;  
    int      si_band;  
    int      si_fd;
} siginfo_t;

struct sigaction {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_trampoline)(void);
};

int raise(int sig) __THROW;
int kill(pid_t pid, int sig) __THROW;

int killpg(pid_t pgrp, int sig) __THROW;

int sigaddset(sigset_t *set, int sig) __THROW;
int sigdelset(sigset_t *set, int sig) __THROW;
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) __THROW;
int sigpending(sigset_t *set) __THROW;

int sigemptyset(sigset_t *set) __THROW;
int sigfillset(sigset_t *set) __THROW;

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

__END_DECLS

#endif