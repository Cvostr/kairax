#include "kairax/signal.h"
#include "cpu/cpu_local.h"
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
    DEFINE_SIGNAL(SIGTSTP, SIG_STOP),
    DEFINE_SIGNAL(SIGTTIN, SIG_STOP),
    DEFINE_SIGNAL(SIGTTOU, SIG_STOP),
    DEFINE_SIGNAL(SIGURG, SIG_IGNORE),
    DEFINE_SIGNAL(SIGXCPU, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGXFSZ, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGVTALRM, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGPROF, SIG_TERMINATE),
    DEFINE_SIGNAL(SIGWINCH, SIG_IGNORE)
    // todo: add more
};

// специфические реализации для разных CPU архитектур
extern void arch_signal_handler(struct thread* thr, int signum, int caller, void* frame);

void process_handle_signals(int caller, void* frame)
{
    struct thread* thread = cpu_get_current_thread();
    struct process* process = thread->process;

    sigset_t sigs = thread->pending_signals;

    if (sigs == 0) {
        return;
    }

    // Найти первый сигнал для обработки
    int signal = 0;
    while (((sigs >> signal) & 1) == 0)
    {
        signal++;
    }

    // Снять сигнал
    thread->pending_signals &= ~(1ULL << signal);

    struct proc_sigact* sigact = &process->sigactions[signal];

    if (sigact->handler == SIG_IGN)
    {
        ;
    } 
    else if (sigact->handler == SIG_DFL)
    {
        // Действие по умолчанию
        int sigdefault = signals_table[signal];

        if (sigdefault == SIG_TERMINATE || sigdefault == SIG_CORE) {
		    exit_process(MAKETERMCODE(signal));
        }
    } 
    else
    {
        arch_signal_handler(thread, signal, caller, frame);
    }
}