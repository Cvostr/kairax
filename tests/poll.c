#include "stdio.h"
#include "process.h"
#include "unistd.h"
#include "time.h"
#include "sched.h"
#include "errno.h"
#include "sys/wait.h"
#include "poll.h"
#include "sys/select.h"

int fds[2];
int fds1[2];

void thread(void* n) {
    sleep(1);

    char val = '0';
    write(fds[1], &val, 1);

    thread_exit(125);
}

int main(int argc, char** argv) {

    char val = '0';
    int rc = pipe(fds);
    rc = pipe(fds1);

    pid_t pid = create_thread(thread, NULL);

    struct pollfd pfds[3];
    pfds[0].fd = fds[0];
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    pfds[1].fd = fds1[0];
    pfds[1].events = POLLIN;
    pfds[1].revents = 0;

    printf("poll()\n");
    int events = poll(pfds, 2, -1);
    printf("got %i events. revents1 = %i revents2 = %i\n", events, pfds[0].revents, pfds[1].revents);
    read(fds[0], &val, 1);

    int status  = 0;
    waitpid(pid, &status,  0);


    printf("Test 2\n");
    write(fds[1], &val, 1);
    write(fds1[1], &val, 1);

    pfds[0].fd = fds[0];
    pfds[0].events = POLLIN;
    pfds[0].revents = 0;
    pfds[1].fd = fds1[0];
    pfds[1].events = POLLIN;
    pfds[1].revents = 0;
    pfds[2].fd = fds[1];
    pfds[2].events = POLLIN | POLLOUT;
    pfds[2].revents = 0;

    printf("poll()\n");
    events = poll(pfds, 3, -1);
    printf("got %i events. revents1 = %i revents2 = %i revents3 = %i\n", events, pfds[0].revents, pfds[1].revents, pfds[2].revents);
    read(fds[0], &val, 1);
    read(fds1[0], &val, 1);

    printf("Test 3\n");
    pfds[0].revents = 0;
    pfds[1].revents = 0;
    pfds[2].revents = 0;

    printf("poll() with timeout 0\n");
    events = poll(pfds, 2, 0);
    printf("got %i events. revents1 = %i revents2 = %i\n", events, pfds[0].revents, pfds[1].revents);

    int TIMEOUT = 2000;
    printf("Test 4\n");
    pfds[0].revents = 0;
    pfds[1].revents = 0;
    pfds[2].revents = 0;

    printf("poll() with timeout %i\n", TIMEOUT);
    events = poll(pfds, 2, TIMEOUT);
    printf("got %i events. revents1 = %i revents2 = %i\n", events, pfds[0].revents, pfds[1].revents);

    return 0;
}