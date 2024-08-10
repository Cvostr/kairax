#include "unistd.h"
#include "sys/wait.h"

int exec_args(char* cmd, char** args) 
{
    pid_t pid = fork();

    if (pid == 0) {
        execvp(cmd, args);
        return 127;
    }

    int status = 0;
    waitpid(pid, &status, 0);

    return status;
}

int main(int argc, char** argv) 
{
    printf("Load rtl8139 module\n");
    char* modctl_args[3] = {"modctl", "/rtl8139.ko", 0};
    printf("Status: %i\n", exec_args("modctl", modctl_args));
    
    printf("Setting IPv4\n");
    char* netctl_args[5] = {"netctl", "eth0", "setip4", "10.0.2.15", 0};
    printf("Status: %i\n", exec_args("netctl", netctl_args));

    printf("Setting Default Route\n");
    char* route_args[3] = {"route", "add", 0};
    printf("Status: %i\n", exec_args("route", route_args));

    return 0;
}