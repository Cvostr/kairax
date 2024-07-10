#include "stdio.h"
#include "unistd.h"
#include "string.h"

int main(int argc, char** argv, char** envp) 
{

    char divisor = '\n';
    for (int i = 1; i < argc; i ++)
    {
        char* arg = argv[i];
        if (strcmp(arg, "-0") == 0 || strcmp(arg, "--null") == 0)
        {
            divisor = '\0';
        }
    }

    if (envp) 
    {
        while (*envp) {
            printf("%s%c", *envp, divisor);
            envp++;
        }
        fflush(stdout);
    }

    return 0;
}