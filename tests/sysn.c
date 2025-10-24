#include "stdio.h"
#include "fcntl.h"
#include "unistd.h"
#include "sys/stat.h"
#include "errno.h"
#include "process.h"
#include "arpa/inet.h"
#include "signal.h"
#include "string.h"
#include "stdlib.h"
#include <termios.h>
#include "threads.h"

char buff[220];
static struct termios old, current;

void initTermios() 
{
    tcgetattr(0, &old);
    current = old;
    current.c_lflag &= ~ICANON;
    current.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &current);
}

void inthandler(int arg)
{
    printf("signal handled with arg %i\n", arg);
	printf("Terminate [y/n]?\n");
	char r;
	scanf("%c", &r);
	printf("input: %c (%i)\n", r, r);
    if (r == 'y')
	{
        exit(0);
	}
}

void segfaulthandle(int arg)
{
    printf("SIGSEGV handled\n");
	exit(1);
}

void resetTermios(void) 
{
    tcsetattr(0, TCSANOW, &old);
}

char getch(void) 
{
    char ch;
    initTermios();
    ch = getchar();
    resetTermios();
    return ch;
}

void thread_func(int * arg) {
    printf("ARG : %i\n", *arg);

    while(1) {
		sleep(4);

        printf("TID %i \t", gettid());
        fflush(stdout);
    }
}

int values[] = { 40, 10, 34, 382, 67, 98, 100, 90, 20, 25 };
int qscompare(const void* a, const void* b)
{
	return (*(int*)a - *(int*)b);
}

int main(int argc, char** argv, char** envp) {

    int counter = 0;
    printf("PID : %i\n", getpid());
    printf("PPID : %i\n", getppid());
    
    getcwd(buff, 220);
    printf("CWD : %s\n", buff);

    printf("ENVS : ENVP %p\n", envp);
    if (envp) {
    while (*envp) {
        printf("%s\n", *envp);
        envp++;
    }
    }

    int fd1 = open("/mydir/blabla", O_RDONLY, 0);
    int fd2 = open("/mydir", O_RDONLY, 0);
    int rc1 = read(fd1, buff, 119);
    close(fd2);

    buff[lseek(fd1, 0, SEEK_CUR)] = '\0';
    printf(buff);
    close(fd1);

    int fd = open("/bugaga.txt", O_RDWR, 0);
    printf("BEFORE READ POS %i\n", lseek(fd, 0, SEEK_CUR));
    int rc = read(fd, buff, 119);
    printf("AFTER READ POS %i\n", lseek(fd, 0, SEEK_CUR));
    lseek(fd, 100, SEEK_SET);
    printf("AFTER SEEK POS %i\n", lseek(fd, 0, SEEK_CUR));

    //
    rc = fcntl(fd, F_GETFL);
    printf("fcntl() GETFL result %i\n", rc);
    rc = fcntl(fd, F_SETFL, O_APPEND | O_DIRECTORY);
    rc = fcntl(fd, F_GETFL);
    printf("fcntl() GETFL 2nd result %i\n", rc);
    //

    rc = fcntl(fd, F_GETFD);
    printf("fcntl() GETFD result %i\n", rc);
    rc = fcntl(fd, F_SETFD, FD_CLOEXEC);
    rc = fcntl(fd, F_GETFD);
    printf("fcntl() GETFD 2nd result %i\n", rc);

    struct stat statbuf;
    rc = fstat(fd, &statbuf);
    printf("TYPE : %i, SIZE : %i, CTIME : %i, MODE : %i\n", statbuf.st_mode, statbuf.st_size, statbuf.st_ctime, statbuf.st_mode);

	buff[119] = '\0';
	printf("Contents of bugaga.txt: %s\n", buff);
	close(fd);

//
    fd = open("/dev/random", O_CLOEXEC, 0);
    rc = fcntl(fd, F_GETFD);
    printf("fcntl() on /dev/random GETFD result %i\n", rc);
    rc = fcntl(fd, F_SETFD, 0);
    rc = fcntl(fd, F_GETFD);
    printf("fcntl() on /dev/random GETFD after unset result %i\n", rc);

    // ошибка
    close(123);
    printf("ERRNO = %i\n", errno);

    sigset_t pending;
    sigpending(&pending);

    printf("inet_addr: %u\n", inet_addr("10.0.2.2"));
    printf("htonl: %u %u %u\n", 33685514, htonl(33685514), htonl(htonl(33685514)));
    printf("htonl: %u %u %u\n", 167772674, htonl(167772674), htonl(htonl(167772674)));

    ///
    qsort(values, 10, sizeof(int), qscompare);
    for (int n = 0; n < 10; n++)
	    printf("%d ", values[n]);
    printf("\n");
    ///

    ///
    const char *string = "abcde312$#@";
    const char *invalid = "*$#";
    size_t valid_len = strcspn(string, invalid);
    if (valid_len != strlen(string))
       printf("'%s' contains invalid chars starting at position %zu\n", string, valid_len);
    ///

    ///
    string = "abcde312$#@";
    const char* low_alpha = "qwertyuiopasdfghjklzxcvbnm";
    size_t spnsz = strspn(string, low_alpha);
    printf("After skipping initial lowercase letters from '%s'\nThe remainder is '%s'\n", string, string + spnsz);
    //

    ///
    char str[] ="- This, a sample string.";
    char * pch;
    printf ("Splitting string \"%s\" into tokens:\n",str);
    pch = strtok (str," ,.-");
    while (pch != NULL)
    {
        printf ("%s\n",pch);
        pch = strtok (NULL, " ,.-");
    }
    ///

    if (argc > 1 && strcmp(argv[1], "getch") == 0)
    {
        printf("getch() test mode\n");
        while (1)
        {
            int cha = getch();
            printf("Pressed key %i\n", cha);
        }
    }

    sighandler_t hd = signal(SIGINT, inthandler);
    if (hd == SIG_ERR)
    {
        perror("signal error");
        return 1;
    }

    if (argc > 1 && strcmp(argv[1], "loop") == 0)
    {
        while (1)
        {
        }
    }

    if (argc > 1 && strcmp(argv[1], "cnd") == 0)
    {
        struct timespec tsp;
        tsp.tv_sec = 7;
        tsp.tv_nsec = 0;
        printf("CND test\n");
        cnd_t cndtest;
        mtx_t mttest;
        mtx_init(&mttest, mtx_plain);
        cnd_init(&cndtest);
        cnd_timedwait(&cndtest, &mttest, &tsp);
    }

    if (argc > 1 && strcmp(argv[1], "paus") == 0)
    {
        while (1)
        {
            int ret = pause();
            printf("result %i should be %i\n", errno, EINTR);
        }
    }

    if (argc > 1 && strcmp(argv[1], "segv") == 0)
    {
        signal(SIGSEGV, segfaulthandle);
        int *__ptr = 0;
        *__ptr = 0;
    }

    int send = 343;
    pid_t tpi = create_thread(thread_func, &send);
    printf("TID %i \t", tpi);

    int iterations = 0;

    while(1) {
		sleep(2);

        printf(" %i", counter++);
        fflush(stdout);
    }

    return 0;
}