#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "kairax/types.h"

#define SIG_BLOCK          0 
#define SIG_UNBLOCK        1
#define SIG_SETMASK        2

#define SIGHUP          1
#define SIGINT          2
#define SIGQUIT         3
#define SIGILL          4
#define SIGTRAP         5
#define SIGABRT         6
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

#define SIGUNUSED	    31

#define SIGNALS         32

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

#define SA_RESTORER	    0x04000000
#define SA_ONSTACK	    0x08000000
#define SA_RESTART	    0x10000000
#define SA_INTERRUPT	0x20000000 /* dummy -- ignored */
#define SA_NODEFER	    0x40000000
#define SA_RESETHAND	0x80000000

void process_handle_signals();

#endif