#include "stdio.h"
#include "sys_files.h"
#include "../sdk/sys/syscalls.h"
#include "errno.h"
#include "process.h"

char buff[220];

void thread_func(int * arg) {
    printf("ARG : %i\n", *arg);

    while(1) {
		sleep(40);

        printf("TID %i \t", thread_get_id());
    }
}

int main() {

    int counter = 0;
    printf("PID : %i\n", process_get_id());
    
    syscall_get_working_dir(buff, 220);
    printf("CWD : %s\n", buff);

    int fd1 = open_file("/mydir/blabla", FILE_OPEN_MODE_READ_ONLY, 0);
    int fd2 = open_file("/mydir", FILE_OPEN_MODE_READ_ONLY, 0);
    int rc1 = read(fd1, buff, 119);
    close(fd2);

    buff[lseek(fd1, 0, SEEK_CUR)] = '\0';
    printf(buff);
    close(fd1);

    int fd = open_file("/bugaga.txt", FILE_OPEN_MODE_READ_ONLY, 0);
    printf("BEFORE READ POS %i\n", lseek(fd, 0, SEEK_CUR));
    int rc = read(fd, buff, 119);
    printf("AFTER READ POS %i\n", lseek(fd, 0, SEEK_CUR));
    lseek(fd, 100, SEEK_SET);
    printf("AFTER SEEK POS %i\n", lseek(fd, 0, SEEK_CUR));

    struct stat statbuf;
    rc = fstat(fd, &statbuf);
    printf("TYPE : %i, SIZE : %i, CTIME : %i, MODE : %i\n", statbuf.st_mode, statbuf.st_size, statbuf.st_ctime, statbuf.st_mode);

	buff[119] = '\0';
	printf(buff);
	close(fd);

    // ошибка
    close(123);
    printf("ERRNO = %i", errno);

    int send = 343;
    create_thread(thread_func, &send, NULL);

    int iterations = 0;

    while(1) {
		sleep(20);

        printf(" %i", counter++);
    }

    return 0;
}