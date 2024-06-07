#include "stdio.h"
#include "assert.h"

int main(int argc, char** argv) {
    printf("Kamikaze asserion test\n");
    int i = 1000;
    assert(i == 1);

    printf("Assertion is missed!\n");
    return -1;
}