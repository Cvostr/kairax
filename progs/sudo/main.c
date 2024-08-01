#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "errno.h"
#include "stdlib.h"

#define PASSWORD "12345"

int main(int argc, char** argv) {

    if (argc < 2)
    {
        printf("Enter program file\n");
        return 1;
    }

/*
    printf("Enter password:\n");
    char passwd[100];
    scanf("%s", passwd);

    if (strcmp(passwd, PASSWORD) != 0)
    {
        printf("Password is incorrect!\n");
        return 1;
    }
*/

    int rc = execvp(argv[1], argv + 1);
    if (errno == ENOENT)
    {
        printf("Command %s not found!\n", argv[1]);
    }

    return 1;
}