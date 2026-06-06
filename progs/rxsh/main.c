#include "string.h"
#include "unistd.h"
#include "stdio.h"
#include "lexer.h"
#include "stdlib.h"
#include "ast.h"
#include "sys/ioctl.h"
#include "signal.h"
#include "getopt.h"
#include "termios.h"
#include "errno.h"

int trace = 0;
char curdir[512];
char hostname[256];
char cmd[256];

void printcwd();
void cmd_process();

// https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_03

void sigint_handler(int sig) {
    // Do nothing or set a global volatile flag
}

ssize_t readline(char *dest, size_t max)
{
    dest[0] = '\0';
    ssize_t res = read(STDIN_FILENO, dest, max - 1);
    switch (res)
    {
    case 0:
        // EOF
        printf("exit\n");
        _exit(0);
        break;
    case -1:
        return -1;
    default:
        dest[res] = '\0';
        break;
    }

    return res;
}

int main(int argc, char** argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "xc:")) != -1) {
        switch (opt) {
            case 'x':
                trace = 1;
                break;
            case 'c':
                strcpy(cmd, optarg); // Аргумент, следующий сразу за -c
                cmd_process();
                return 0;
        }
    }

    setpgid(0, 0);
    ioctl(0, TIOCSPGRP, getpgid(0));

/*
    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, SIGINT);
    sigprocmask(SIG_BLOCK, &ss, NULL);
*/

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error registering sigaction");
        exit(EXIT_FAILURE);
    }

    while (1) 
    {
        printcwd();
        
        ssize_t len = readline(cmd, sizeof(cmd));
        if (len == -1)
        {
            if (errno == EINTR)
                printf("\n");
        }

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
    struct token* tokens[20];
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

    // Освободить память под токены - не нужны больше
    for (int i = 0; i < n; i ++)
    {
        free(tokens[i]);
    }    

    // Выполнение собранных AST
    struct list_node* anode = root->nodes->head;
    while (anode != NULL) 
    {
        struct ast_node* nd = anode->element;
        ast_execute(nd);

        anode = anode->next;
    }

    // очистить AST дерево
    ast_free(root);
}