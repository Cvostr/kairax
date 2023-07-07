#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/sys/syscalls.h"
#include "../sdk/libc/errno.h"
#include "../sdk/libc/process.h"

char buff[120];

void thread_func(int * arg) {
    printf("ARG : %i\n", *arg);

    while(1) {
		sleep(40);

        printf("TID %i \t", syscall_thread_get_id());
    }
}

int main() {

    int counter = 0;
    printf("PID : %i\n", process_get_id());

    int send = 343;
    create_thread(thread_func, &send, NULL);

    int iterations = 0;

    while(1) {
		sleep(20);

        printf(" %i", counter++);
    }

    return 0;
}