#include "stdio.h"
#include "unistd.h"

int main() {
    printf("\033[2J");
    fflush(NULL);
    return 0;
}