#include "../sdk/libc/stdio.h"
#include "../sdk/libc/sys_files.h"
#include "../sdk/sys/syscalls.h"

char buff[120];

int main() {

    int counter = 0;
    printf("PID : %i\n", syscall_process_get_id());

	int fd = open_file("/bugaga.txt", FILE_OPEN_MODE_READ_ONLY, 0);
    int fd1 = open_file("/bugaga.txt", FILE_OPEN_MODE_READ_ONLY, 0);
	int rc = read(fd, buff, 119);

	struct stat fstat;
    rc = syscall_fdstat(fd, &fstat);
	printf("TYPE : %i, SIZE : %i, CTIME : %i\n", fstat.st_mode, fstat.st_size, fstat.st_ctime);

	buff[119] = '\0';
	printf(buff);
	close(fd);

    int iterations = 0;

    while(1) {
		syscall_sleep(20);

        printf(" %i", counter++);
    }

    return 0;
}