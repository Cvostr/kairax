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

#define SIGUNUSED	31

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
    void (*sa_restorer)(void);
};

void process_handle_signals();

#endif