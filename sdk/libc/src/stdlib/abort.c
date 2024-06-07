#include "stdlib.h"
#include "signal.h"
#include "stdio.h"

void abort()
{
    fflush(NULL);

    sigset_t t;
    sigaddset(&t, SIGABRT);
    sigprocmask(SIG_UNBLOCK, &t, 0);

    if (raise(SIGABRT) != 0)
        exit(127);
}