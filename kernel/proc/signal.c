#include "kairax/signal.h"
#include "cpu/cpu_local_x64.h"
#include "syscalls.h"

#define SIG_IGNORE 0
#define SIG_TERMINATE   1
#define SIG_CORE        2
#define SIG_STOP        3

#define DEFINE_SIGNAL(sig, type) [sig] = type

int signals_table[SIGNALS] = {

    DEFINE_SIGNAL(SIGHUP, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGINT, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGQUIT, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGILL, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGTRAP, SIG_CORE),
    DEFINE_SIGNAL(SIGABRT, SIG_CORE),
    DEFINE_SIGNAL(SIGBUS, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGFPE, SIG_CORE),
    DEFINE_SIGNAL(SIGKILL, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGUSR1, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGSEGV, SIG_CORE),
    DEFINE_SIGNAL(SIGUSR2, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGPIPE, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGALRM, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGTERM, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGSTKFLT, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGCHLD, SIG_IGNORE),
    DEFINE_SIGNAL(SIGCONT, SIG_IGNORE),
    DEFINE_SIGNAL(SIGSTOP, SIG_STOP),
    DEFINE_SIGNAL(SIGTSTP, SIG_STOP)
    // todo: add more
};

void process_handle_signals()
{
    struct process* process = cpu_get_current_thread()->process;
    if (try_acquire_spinlock(&process->sighandler_lock)) {

        sigset_t sigs = process->pending_signals;
        process->pending_signals = 0;

        if (sigs == 0) {
            goto exit;
        }

        int signal = 0;
        while (sigs > 0) {
            
            if ((sigs & 1) == 1) {
                int sigdefault = signals_table[signal];

                if (sigdefault == SIG_TERMINATE || sigdefault == SIG_CORE) {
			        sys_exit_process(128 + signal);
                }
            }

            sigs >>= 1;
            signal++;
        }
    exit:
        release_spinlock(&process->sighandler_lock);
    }
}