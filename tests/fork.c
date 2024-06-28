#include "stdio.h"
#include "assert.h"
#include "string.h"
#include "unistd.h"
#include "sys/wait.h"

int main(int argc, char** argv) 
{

    printf("Fork() test program\n");

    pid_t r = fork();
    if (r == 0) {
        printf("CHILD %i\n", getpid());

        char* args[3];
        args[0] = "ls.a";
        args[1] = "-a";
        args[2] = NULL;

        int rc = execve("/ls.a", args, NULL);
        printf("exec() :%i\n", rc);
        return 22;
    } else if (r > 0) {
        printf("PARENT, child: %i\n", r);
        printf("Waiting for completion\n");
        int status = 0;
        waitpid(r, &status, 0);
        printf("Completed with code %i\n", status);
    } else {
        perror("fork()");
    }

}