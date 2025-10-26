#include "stdio.h"
#include "process.h"
#include "unistd.h"
#include "time.h"
#include "sched.h"
#include "errno.h"
#include "sys/wait.h"
#include "poll.h"
#include "sys/select.h"
#include "string.h"

int fds[2];
int fds1[2];

void thread(void* n) {
    sleep(1);

    char val = '0';
    write(fds[1], &val, 1);

    thread_exit(125);
}

void thread_closefd(void* n) {
    sleep(1);

    int* f = n;
    close(*f);

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

    printf("Test 5\n");
    struct pollfd pfdWrong;
    pfdWrong.fd = 20;
    pfdWrong.events = POLLIN | POLLOUT;
    printf("poll() with incorrect fd\n");
    events = poll(&pfdWrong, 1, TIMEOUT);
    printf("got %i events. revents1 = %i\n", events, pfdWrong.revents);

    printf("Test 6\n");
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fds[0], &readfds);

    pid = create_thread(thread, NULL);

    printf("select()\n");
    events = select(fds[0] + 1, &readfds, NULL, NULL, NULL);
    printf("got %i events\n", events);
    read(fds[0], &val, 1);

    waitpid(pid, &status,  0);

    int FDSC = 10000000;
    printf("Test 7\n");
    printf("select() with %i fds\n", FDSC);
    events = select(FDSC, &readfds, NULL, NULL, NULL);
    printf("got %i events, errno = %i\n", events, errno);

    printf("Test 8\n");
    printf("select() with timeout %i\n", TIMEOUT);
    struct timeval stim;
    stim.tv_sec = TIMEOUT / 1000;
    stim.tv_usec = 0;
    events = select(fds[0] + 1, &readfds, NULL, NULL, &stim);
    printf("got %i events\n", events);



    struct pollfd closepfd;
    closepfd.fd = fds[0];
    closepfd.events = 0;
    printf("Test 9\n");

    pid = create_thread(thread_closefd, &fds[1]);
    printf("poll() with pipe with closing write end\n");
    events = poll(&closepfd, 1, -1);
    printf("got %i events. revents1 = %i\n", events, closepfd.revents);
    waitpid(pid, &status,  0);

    return 0;
}