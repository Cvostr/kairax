#include "bootshell.h"
#include "dev/keyboard/int_keyboard.h"
#include "bootshell_cmdproc.h"
#include "stdio.h"
#include "dev/b8-console/b8-console.h"

char command[256];
int cmd_len = 0;

char curdir[512];
struct inode* wd_inode = NULL;
struct dentry* wd_dentry = NULL;

void bootshell_print_sign()
{
    printf("KAIRAX:%s# ", curdir);
}

void bootshell()
{
    bootshell_print_sign();

    while(1){
        char keycode = keyboard_get_key();
        char symbol = keyboard_get_key_ascii(keycode);

        if(symbol != 0 && symbol != '\n'){
            printf("%c", symbol);

            command[cmd_len++] = symbol;
        }

        if(symbol == '\n'){
            command[cmd_len++] = '\0';
            printf("%c", symbol);
            bootshell_process_cmd(command);
            bootshell_print_sign();
            cmd_len = 0;
        }

        if(keycode == 15 && cmd_len > 0){ //удаление символа
            cmd_len--;
            b8_remove_from_end(1);
        }
    }
}