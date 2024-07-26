#include <sys/reboot.h>
#include <string.h>
#include "stdio.h"

int main(int argc, char** argv)
{
    int cmd = REBOOT_CMD_POWER_OFF;

    for (int i = 1; i < argc; i ++)
    {
        if (strcmp(argv[i], "-r") == 0) 
        {
            cmd = REBOOT_CMD_REBOOT;
        } else {
            printf("Wrong parameter: %s\n", argv[i]);
            return -1;
        }
    }

    int rc = reboot(cmd);
    if (rc != 0) 
    {
        perror("Operation Failed");
        return -1;
    }

    return 0;
}