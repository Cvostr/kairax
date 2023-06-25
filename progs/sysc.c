#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/sys/syscalls.h"

#define BSS_LEN 10000

int big_array[BSS_LEN];

int main() {

    for(int i = 0; i < BSS_LEN; i ++)
        big_array[i] = i;

    //memset(big_array, 12, sizeof(int) * BSS_LEN);

    char str[3];
    str[0] = 'A';
    str[1] = '\n';
    str[2] = 0;

    printf("PID : %i", syscall_process_get_id());

    int iterations = 0;

    for(iterations = 0; iterations < 10; iterations ++) {

        syscall_sleep(20);

        str[0]++;

        printf("Hello from program: %s", str);
    }

    return 0;
}