#include "signal.h"
#include "stdlib.h"
#include "errno.h"
#include "stdio.h"
#include "string.h"

#define DEFINE_SIGNAL(sig) [sig] = #sig

char* signals_table[NSIG] = {
    DEFINE_SIGNAL(SIGHUP),
    DEFINE_SIGNAL(SIGINT),
    DEFINE_SIGNAL(SIGQUIT),
    DEFINE_SIGNAL(SIGILL),
    DEFINE_SIGNAL(SIGTRAP),
    DEFINE_SIGNAL(SIGABRT),
    DEFINE_SIGNAL(SIGBUS),
    DEFINE_SIGNAL(SIGFPE),
    DEFINE_SIGNAL(SIGKILL),
    DEFINE_SIGNAL(SIGUSR1),
    DEFINE_SIGNAL(SIGSEGV),
    DEFINE_SIGNAL(SIGUSR2),
    DEFINE_SIGNAL(SIGPIPE),
    DEFINE_SIGNAL(SIGALRM),
    DEFINE_SIGNAL(SIGTERM),
    DEFINE_SIGNAL(SIGSTKFLT),
    DEFINE_SIGNAL(SIGCHLD),
    DEFINE_SIGNAL(SIGCONT),
    DEFINE_SIGNAL(SIGSTOP),
    DEFINE_SIGNAL(SIGTSTP),
    DEFINE_SIGNAL(SIGTTIN),
    DEFINE_SIGNAL(SIGTTOU),
    DEFINE_SIGNAL(SIGURG),
    DEFINE_SIGNAL(SIGXCPU),
    DEFINE_SIGNAL(SIGXFSZ),
    DEFINE_SIGNAL(SIGVTALRM),
    DEFINE_SIGNAL(SIGPROF),
    DEFINE_SIGNAL(SIGWINCH)
    // todo: add more
};

void print_signals_list()
{
    int i, j;
    int cntr = 0;
    size_t len = 0;
    size_t maxlen = 0;

    // Сначала считаем сигнал с самым длинным названием
    for (i = 1; i < NSIG; i ++)
    {
        len = strlen(signals_table[i]);
        
        if (len > maxlen)
            maxlen = len; 
    }

    // Выводим список
    for (i = 1; i < NSIG; i ++)
    {
        char* signame = signals_table[i];
        len = strlen(signame);
        printf("%2i) %s", i, signame);

        if (++cntr == 5)
        {
            // В одной строке по 5 сигналов
            cntr = 0;
            printf("\n");
        } 
        else
        {
            // Добавляем пробелы для выравнивания вывода
            for (j = 0; j < maxlen - len + 1; j++)
            {
                printf(" ");
            }
        }
    }
    
}

int main(int argc, char** argv) {

    int i = 0;
    int rc = 0;
    int sig = SIGTERM;
    pid_t pid = 0;

    pid_t* pgids = malloc(argc);
    int npgids = 0;
    pid_t* pids = malloc(argc);
    int npids = 0;

    int arg = 1;
    while (arg < argc) 
    {    
        char* carg = argv[arg];

        if (strcmp(carg, "-l") == 0)
        {
            print_signals_list();
            return 0;
        }

        int ival = atoi(carg);

        if (ival < 0) {
            if (sig == SIGTERM) {
                sig = ival * -1;
            } else {
                pgids[npgids++] = ival * -1;
            }
        } else {
            pids[npids++] = ival;
        }
        arg++;
    }

    if (npids == 0 && npgids == 0) 
    {
        printf("Usage: \n");
        printf("kill -signum pid");
        //todo: actual
        return 0;
    }

    for (i = 0; i < npids; i ++) {
        rc = kill(pids[i], sig);
        if (rc != 0) {
            printf("kill: (%i): errno = %i\n", pids[i], errno);
        }
    }

    for (i = 0; i < npgids; i ++) {
        //rc = kill(pgids[i], sig);
    }

    return 0;
}