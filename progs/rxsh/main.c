#include "string.h"
#include "unistd.h"
#include "stdio.h"
#include "lexer.h"
#include "stdlib.h"
#include "ast.h"
#include "sys/ioctl.h"
#include "signal.h"

char curdir[512];
char hostname[256];
char cmd[256];

void printcwd();
void cmd_process();

// https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_03

int main(int argc, char** argv)
{
    setpgid(0, 0);
    ioctl(0, TIOCSPGRP, getpgid(0));

    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, SIGINT);
    sigprocmask(SIG_BLOCK, &ss, NULL);

    while (1) {
        printcwd();
        fgets(cmd, 100, stdin);
        cmd_process();
    }
    return 0;
}

void printcwd()
{
    uid_t uid = getuid();
    char sign = uid == 0 ? '#' : '$';
    memset(curdir, 0, sizeof(curdir));
    getcwd(curdir, 512);
    gethostname(hostname, 256);
    printf("\033[1;92m%i@%s\033[0m:\033[1;94m%s\033[0m%c ", uid, hostname, curdir, sign);
    fflush(stdout);
}

void cmd_process()
{
    struct token* tokens[10];
    int n = lexer_decode(cmd, strlen(cmd), tokens);

#if 0
    for (int i = 0; i < n; i ++)
    {
        struct token* tk = tokens[i];
        switch (tk->type)
        {
            case TOKEN_STRING:
                printf("TOKEN STR: %s\n", tk->str_val);
                break;
            case TOKEN_CTRL_CHAR:
                printf("TOKEN CTRL: %c\n", tk->punct_val);
                break;
        }
    }
#endif

    if (n == 0) {
        return;
    }

    struct ast_node* root = ast_build(tokens, n);

    list_node* anode = root->nodes->head;
    while (anode != NULL) {
        struct ast_node* nd = anode->element;
        
        ast_execute(nd);

        anode = anode->next;
    }

    for (int i = 0; i < n; i ++)
    {
        free(tokens[i]);
    }    
}