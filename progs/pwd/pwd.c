#include "stdio.h"
#include "unistd.h"

int main() {
    char buffer[1024];
    getcwd(buffer, 1024);
    printf("%s\n", buffer);
}