#include "stdio.h"
#include "process.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

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

    char bf[100];
    __dtostr(1.5, bf, 100, 10, 6, 0);

    char* a = malloc(10);
    char b[10];
    int nv;
    int xv;
    int neg;
    int oct;
    sscanf("123abab 456asas 7891011 FFFF -234 12", "%s %s %i %x %i %o", a, b, &nv, &xv, &neg, &oct);
    printf("1: %s 2: %s 3 NUM: %i 4: %i 5: %i 6: %i\n", a, b, nv, xv, neg, oct);

    sscanf("   0xFFF, 0xDE - test 77", "   0x%x, 0x%x - test %o", &nv, &xv, &oct);
    printf("1: %i 2: %x 3: %o 4: %f 5: %G\n", nv, xv, oct, 123.123456, 232.90921);

    char sym = 'A';

    printf("PID : %i", getpid());

    for(int iterations = 0; iterations < 5; iterations ++) {

        sleep(5);

        sym++;

        printf("From prog: %c", sym);
    }

    return 0;
}