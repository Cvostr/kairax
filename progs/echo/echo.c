#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"

const char SPACE = ' ';
const char NL = '\n';

int main(int argc, char** argv) 
{
    for (int i = 1; i < argc; i ++) 
    {
        write(STDOUT_FILENO, argv[i], strlen(argv[i]));
        if (i < (argc - 1)) {
            write(STDOUT_FILENO, &SPACE, 1);
        }
    }

    write(STDOUT_FILENO, &NL, 1);

    return 0;
}