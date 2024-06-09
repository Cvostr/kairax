#include "stdio.h"
#include "assert.h"
#include "string.h"

int main(int argc, char** argv) {

    if (argc == 2) {
        if (strcmp(argv[1], "seg") == 0) {
            *((char*) 0) = 1;
        }
    }

    printf("Kamikaze asserion test\n");
    int i = 1000;
    assert(i == 1);

    printf("Assertion is missed!\n");
    return -1;
}