#include "stdio.h"
#include "fcntl.h"
#include "unistd.h"
#include "sys/stat.h"
#include "errno.h"
#include "arpa/inet.h"
#include <signal.h>
#include "string.h"
#include "stdlib.h"
#include <termios.h>
#include "threads.h"
#include "sched.h"
#include "libgen.h"
#include "sys/resource.h"
#include "sys/wait.h"

void segfaulthandle(int arg)
{
    printf("SIGSEGV handled\n");
	exit(1);
}

void pipehandle(int arg)
{
    printf("SIGPIPE handled\n");
}

typedef struct PACKED {

    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9; 
    uint64_t r8; 
    
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp; 
    uint64_t ss;
} thread_frame_t;
#define GDT_BASE_KERNEL_CODE_SEG    0x8
#define GDT_BASE_KERNEL_DATA_SEG    0x10
void srophandle(int arg)
{
    uint64_t rsp;
    asm volatile ("mov %%rsp, %%rax\n mov %%rax, %0" : "=m" (rsp));
    printf("RSP %p\n", rsp);
    thread_frame_t *ctx = (thread_frame_t *)(rsp + 0x20 + 8 + 8);
    printf("CS %i SS %i\n", ctx->cs, ctx->ss);
    ctx->cs = GDT_BASE_KERNEL_CODE_SEG;
    ctx->ss = GDT_BASE_KERNEL_DATA_SEG;
}

void chldhandle(int arg)
{
    printf("SIGCHLD handled arg %i\n", arg);

    int status;
    pid_t p = waitpid(-1, &status, WNOHANG);

    printf("Exited process %i with status %i\n", p, WEXITSTATUS(status));
}

void inthandler(int arg)
{
    printf("signal handled with arg %i\n", arg);
    raise(SIGFPE);
	printf("Terminate [y/n]?\n");
	char r;
	scanf("%c", &r);
	printf("input: %c (%i)\n", r, r);
    if (r == 'y')
	{
        exit(0);
	}
}

int main(int argc, char** argv, char** envp) 
{
    int nodefer = 0;
    if (argc > 1 && strcmp(argv[1], "segv") == 0)
    {
        signal(SIGSEGV, segfaulthandle);
        int *__ptr = 0;
        *__ptr = 0;
    }

    if (argc > 1 && strcmp(argv[1], "nodefer") == 0)
    {
        nodefer = 1;
    }

    if (argc > 1 && strcmp(argv[1], "srop") == 0)
    {
        printf("SROP TEST\n");
        signal(SIGINT, srophandle);
        raise(SIGINT);

        // пробуем испортить память ядра
        char *kernel_ptr = (char *)0xFFFFFFFF80000000;
        memset(kernel_ptr, 0, 10000);

        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "pipe") == 0)
    {
        signal(SIGPIPE, pipehandle);
        int pipefds[2];
        pipe(pipefds);
        close(pipefds[0]);
        char ch = 3;
        ssize_t res = write(pipefds[1], &ch, 1);
        printf("write to closed read end returned rc=%i, errno=%i\n", res, errno);
    }

    if (argc > 1 && strcmp(argv[1], "chld") == 0)
    {
        signal(SIGCHLD, chldhandle);
        pid_t ch = fork();
        if (ch == 0)
        {
            printf("Hello from child\n");
            sleep(1);
            exit(55);
        }
        printf("Created process %i\n", ch);
    }

    struct sigaction sact, oldact;
    sact.sa_flags = nodefer == 1 ? SA_NODEFER : 0;
    sact.sa_handler = inthandler;
    sigemptyset(&sact.sa_mask);
    sigaddset(&sact.sa_mask, SIGFPE);

    if (sigaction(SIGINT, &sact, &oldact) != 0)
    {
        perror("signal error");
        return 1;
    }

    sigset_t testset;
    sigemptyset(&testset);
    sigaddset(&testset, 2);
    sigaddset(&testset, 4);
    printf("sigismember %i %i %i %i\n", 
    sigismember(&testset, 1), sigismember(&testset, 2), sigismember(&testset, 3), sigismember(&testset, 4));

    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, SIGHUP);
    sigprocmask(SIG_BLOCK, &ss, NULL);

    while (1)
    {
        printf("Sigs program echo\n");
        sleep(2);
    }
}