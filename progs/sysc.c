#include "stdio.h"
#include "process.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char** argv) {

    printf("ARGS %i\n", argc);
    for(int i = 0; i < argc; i ++) {
        printf("%i : %s \n", i, argv[i]);
    }

    char* testmem;
    for (int i = 0; i < 10; i ++) {
        testmem = malloc(120);
        memset(testmem, 0, 120);
        sprintf(testmem, "String in malloced memory %i %c", 12, 'a');
        printf("%s 0x%x \n", testmem, testmem);
    }

    char a[10];
    char b[10];
    sscanf("123 456", "%s %s", a, b);
    printf("%s %s\n", a, b);

    char str[3];
    str[0] = 'A';
    str[1] = '\n';
    str[2] = 0;

    printf("PID : %i", process_get_id());

    int iterations = 0;

    for(iterations = 0; iterations < 5; iterations ++) {

        sleep(20);

        str[0]++;

        printf("Hello from program: %s", str);
    }

    return 0;
}