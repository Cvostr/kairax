#include "bootshell.h"
#include "dev/keyboard/int_keyboard.h"
#include "bootshell_cmdproc.h"
#include "stdio.h"
#include "drivers/video/video.h"
#include "proc/syscalls.h"

char command[256];

char curdir[512];
struct inode* wd_inode = NULL;
struct dentry* wd_dentry = NULL;

void bootshell_print_sign()
{
    printf_stdout("KAIRAX:%s# ", curdir);
}

void bootshell()
{
    bootshell_print_sign();

    while(1) {

        kfgets(command, 256);

        bootshell_process_cmd(command);
        bootshell_print_sign();

    }
}