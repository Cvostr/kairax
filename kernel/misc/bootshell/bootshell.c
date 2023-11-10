#include "bootshell.h"
#include "dev/keyboard/int_keyboard.h"
#include "bootshell_cmdproc.h"
#include "stdio.h"
#include "drivers/video/video.h"
#include "proc/syscalls.h"

char command[256];
int cmd_len = 0;

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

        char symbol = getch();
        if (symbol != 0 && symbol != '\n') {
            command[cmd_len++] = symbol;
        }

        if (symbol == '\n') {
            command[cmd_len++] = '\0';
            bootshell_process_cmd(command);
            bootshell_print_sign();
            cmd_len = 0;
        }

        /*if (symbol == 8 && cmd_len > 0) { //удаление символа
            cmd_len--;
            console_remove_from_end(1);
        }*/
    }
}