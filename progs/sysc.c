#include "stdio.h"
#include "sys_files.h"
#include "process.h"

#define BSS_LEN 10000

int big_array[BSS_LEN];

int main(int argc, char** argv) {

    printf("ARGS %i\n", argc);
    for(int i = 0; i < argc; i ++) {
        printf("%i : %s \n", i, argv[i]);
    }

    for(int i = 0; i < BSS_LEN; i ++)
        big_array[i] = i;

    //memset(big_array, 12, sizeof(int) * BSS_LEN);

    char str[3];
    str[0] = 'A';
    str[1] = '\n';
    str[2] = 0;

    printf("PID : %i", process_get_id());

    int iterations = 0;

    for(iterations = 0; iterations < 10; iterations ++) {

        sleep(20);

        str[0]++;

        printf("Hello from program: %s", str);
    }

    return 0;
}