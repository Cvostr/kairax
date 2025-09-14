#include "signal.h"
#include "stdlib.h"
#include "errno.h"
#include "stdio.h"

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
        int ival = atoi(argv[arg]);

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