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

struct ast_exec_ctx {
    int out;
    int in;
    int err;
    pid_t child;
    int nowait;
};

void ast_execute_node(struct ast_node* node, struct ast_exec_ctx *ctx);
void ast_exec_func(struct ast_node* node, struct ast_exec_ctx *ctx);
void ast_exec_pipe(struct ast_node* node, struct ast_exec_ctx *ctx);

void ast_execute(struct ast_node* node)
{       
    struct ast_exec_ctx sender_ctx;
    sender_ctx.out = -1;
    sender_ctx.in = -1;
    sender_ctx.err = -1;
    sender_ctx.nowait = 0;

    ast_execute_node(node, &sender_ctx);
}

void ast_execute_node(struct ast_node* node, struct ast_exec_ctx *ctx)
{
    switch (node->type)
    {
    case AST_NODE_FUNC:
        ast_exec_func(node, ctx);
        break;
    case AST_NODE_PIPE:
        ast_exec_pipe(node, ctx);
        break;
    default:
        break;
    }
}

void ast_exec_pipe(struct ast_node* node, struct ast_exec_ctx *ctx)
{
    int proc_status;
    int pipefd[2];
    int rc = pipe(pipefd);
    if (rc == -1)
    {
        printf("error alloc pipe %i\n", errno);
        return;
    }

    struct ast_exec_ctx sender_ctx;
    sender_ctx.out = pipefd[1];
    sender_ctx.in = -1;
    sender_ctx.err = -1;
    sender_ctx.nowait = 1;

    struct ast_exec_ctx receiver_ctx;
    receiver_ctx.out = ctx->out;        // для многоуровневых перенаправлений (cmd | cmd | cmd)
    receiver_ctx.in = pipefd[0];
    receiver_ctx.err = -1;
    receiver_ctx.nowait = 1;

    ast_execute_node(node->pipe.sender, &sender_ctx);
    close(pipefd[1]);

    ast_execute_node(node->pipe.receiver, &receiver_ctx);
    close(pipefd[0]);

    waitpid(sender_ctx.child, &proc_status, 0);
    //printf("Sender Status %i\n", proc_status);

    waitpid(receiver_ctx.child, &proc_status, 0);
    //printf("Status %i\n", proc_status);
}

void ast_exec_func(struct ast_node* node, struct ast_exec_ctx *ctx)
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
            // обработаем перенаправления
            if (ctx->out != -1)
            {
                dup2(ctx->out, STDOUT_FILENO);
                close(ctx->out);
            }

            if (ctx->in != -1)
            {
                dup2(ctx->in, STDIN_FILENO);
                close(ctx->in);
            }

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

            // сохранить PID в контекст
            ctx->child = pid;

            if (ctx->nowait == 0)
            {
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
}