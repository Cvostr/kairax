#include "stdio.h"
#include "fcntl.h"
#include "unistd.h"
#include "sys/stat.h"
#include "errno.h"
#include "process.h"
#include "arpa/inet.h"

char buff[220];

void thread_func(int * arg) {
    printf("ARG : %i\n", *arg);

    while(1) {
		sleep(4);

        printf("TID %i \t", gettid());
        fflush(stdout);
    }
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

    printf("inet_addr: %u\n", inet_addr("10.0.2.2"));
    printf("htonl: %u %u %u\n", 33685514, htonl(33685514), htonl(htonl(33685514)));
    printf("htonl: %u %u %u\n", 167772674, htonl(167772674), htonl(htonl(167772674)));

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