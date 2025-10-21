#include "stdio.h"
#include "process.h"
#include "unistd.h"
#include "time.h"
#include "sched.h"
#include "errno.h"
#include "sys/wait.h"
#include "poll.h"

int fds[2];
int fds1[2];

void thread(void* n) {

    sleep(1);

    char val = '0';
    for (int i = 0; i < 7; i ++)
    {
        write(fds[1], &val, 1);
        val += 1;
        if (val == '9')
            val = '0';
    }

    close(fds[1]);

    thread_exit(125);
}

int main(int argc, char** argv) {

    int rc = pipe(fds);
    rc = pipe(fds1);

    pid_t pid = create_thread(thread, NULL);

    struct pollfd pfds[2];
    pfds[0].fd = fds[0];
    pfds[0].events = POLLIN;
    pfds[1].fd = fds1[0];
    pfds[1].events = POLLIN;

    printf("poll()\n");
    int events = poll(pfds, 2, 0);

    printf("got %i events\n", events);

    int status  = 0;
    waitpid(pid, &status,  0);



    return 0;
}