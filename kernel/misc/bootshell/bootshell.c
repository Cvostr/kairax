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

    while(1){
        char keycode = keyboard_get_key();
        char symbol = keyboard_get_key_ascii(keycode);

        if (symbol != 0 && symbol != '\n') {
            printf_stdout("%c", symbol);

            command[cmd_len++] = symbol;
        }

        if (symbol == '\n') {
            command[cmd_len++] = '\0';
            printf_stdout("%c", symbol);
            bootshell_process_cmd(command);
            bootshell_print_sign();
            cmd_len = 0;
        }

        if (keycode == 15 && cmd_len > 0) { //удаление символа
            cmd_len--;
            console_remove_from_end(1);
        }
    }
}