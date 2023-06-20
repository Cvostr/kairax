extern void printf(char*);
#include "../sdk/sys/kairax.h"


static char destination[32] = {0};

char* itoa(long long number, int base){

 	int count = 0;
  do {
      	int digit = number % base;
      	destination[count++] = (digit > 9) ? digit - 10 + 'A' : digit + '0';
    	} while ((number /= base) != 0);
    	destination[count] = '\0';
    	int i;
    	for (i = 0; i < count / 2; ++i) {
      	char symbol = destination[i];
      	destination[i] = destination[count - i - 1];
      	destination[count - i - 1] = symbol;
    	}
    	return destination;

}

char buff[120];

int main() {

    printf("PID: ");
    int counter = 0;
    printf(itoa(syscall_process_get_id(), 10));

	int fd = syscall_open_file("/bugaga.txt", FILE_OPEN_MODE_READ_ONLY, 0);
	int rc = syscall_read(fd, buff, 119);
	buff[119] = '\0';
	printf(buff);
	syscall_close(fd);

    int iterations = 0;

    while(1) {
		syscall_sleep(20);

        printf(" ");
        printf(itoa(counter++, 10));
    }

    return 0;
}