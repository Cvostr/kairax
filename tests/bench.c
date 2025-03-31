#include "sys/wait.h"
#include "stdio.h"
#include <sys/types.h>
#include "stdint.h"
#include "time.h"
#include "syscalls.h"

void proc()
{
    double a = 1.234;
    while (a < 10000002)
    {
        a *= 1.0001;
        syscall_get_ticks_count();
    }

    while (a < 10000002)
    {
        a *= a;
        syscall_get_ticks_count();
    }
}

void thr()
{
    proc();
    thread_exit(125);
}

unsigned long long current_timestamp() {
    return syscall_get_ticks_count();
}

int main()
{
    uint64_t starttime = current_timestamp();
    
    proc();
    proc();

    uint64_t endtime = current_timestamp();
    printf("Singlethread time: %u\n", endtime - starttime);

    starttime = current_timestamp();
    pid_t tpi = create_thread(thr, NULL);
	proc();
    int status = 0;
    int rc = waitpid(tpi, &status, 0);
    endtime = current_timestamp();

    printf("Multithread time: %u\n", endtime - starttime);

	return 0;
}
