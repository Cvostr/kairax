#include "ast.h"
#include "string.h"
#include "unistd.h"
#include "sys/wait.h"
#include "stdlib.h"
#include "signal.h"
#include "errno.h"
#include "stdio.h"
#include "sys/ioctl.h"
#include "fcntl.h"

extern char curdir[512];
extern char** __environ;

void ast_exec_func(struct ast_node* node);

void ast_execute(struct ast_node* node)
{
    switch (node->type)
    {
    case AST_NODE_FUNC:
        ast_exec_func(node);
        break;
    case AST_NODE_PIPE:
        printf("Pipes are not implemented\n");
        break;
    default:
        break;
    }
}

void ast_exec_func(struct ast_node* node)
{
    struct list_node* cur_arg = node->func_call.args.head;
    int argc = node->func_call.args.size;
    char* func = cur_arg->element;
    int rc = 0;

    if (strcmp(func, "cd") == 0) {
        if (argc == 1) {
            return;
        }
        struct list_node* next_arg = node->func_call.args.head->next;
        rc = chdir(next_arg->element);
        if (rc != 0)
        {
            perror("rxsh: cd");
            return;
        }
    } else if (strcmp(func, "export") == 0) {

        if (argc == 1) {
            char** envp = __environ;
            printf("environ %lu\n", __environ);
            while (*envp) {
                printf("%s\n", *envp);
                envp++;
            }
            return;
        }

        struct list_node* next_arg = node->func_call.args.head->next;
        putenv(next_arg->element);
    } else if (strcmp(func, "pwd") == 0) {
        printf("%s\n", curdir);
    } else if (strcmp(func, "exit") == 0) {
        _exit(0);
    } else {
        pid_t pid = fork();

        if (pid == 0) 
        {
            // обработать перенаправления
            struct redirect *redir_curr = node->redir_head;
            while (redir_curr != NULL)
            {
                int fd = open(redir_curr->fname, O_CREAT | O_WRONLY, 0666);
                if (fd == -1) {
                    printf("rxsh: %s: %s", redir_curr->fname, strerror(errno));
                    _exit(127);
                }
                
                dup2(fd, redir_curr->fd);
                close(fd);
                redir_curr = redir_curr->next;
            }

            sigset_t ss;
            sigemptyset(&ss);
            sigaddset(&ss, SIGINT);
            sigprocmask(SIG_UNBLOCK, &ss, NULL);
            
            int args_size = sizeof(char*) * (argc + 1);
            char** args = malloc(args_size);
            memset(args, 0, args_size);
            for (int i = 0; i < argc; i ++) {
                char* arg = list_get(&node->func_call.args, i);
                args[i] = malloc(strlen(arg) + 1);
                strcpy(args[i], arg);
            }
            
            rc = execvp(args[0], args);
            perror("exec error");
            _exit(127);
        } else {
            //ioctl(0, TIOCSPGRP, pid);

            int status = 0;
            waitpid(pid, &status, 0);
            if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                switch (sig) {
                    case SIGABRT:
                        printf("Aborted\n");
                        break;
                    case SIGSEGV:
                        printf("Segmentation fault\n");
                        break;
                }
            }
        }
    }
}