#include "bootshell.h"
#include "dev/keyboard/int_keyboard.h"
#include "bootshell_cmdproc.h"
#include "stdio.h"
#include "drivers/video/video.h"
#include "proc/syscalls.h"

void bootshell_print_sign()
{
    char curdir[512];
    memset(curdir, 0, sizeof(curdir));
    sys_get_working_dir(curdir, 512);
    printf_stdout("KAIRAX:%s# ", curdir);
}

void bootshell()
{
    bootshell_print_sign();

    while(1) {

        char command[256];
        kfgets(command, 256);

        bootshell_process_cmd(command);
        bootshell_print_sign();

    }
}