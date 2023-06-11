extern void printf(char*);
extern int syscall_process_get_pid();

#define BSS_LEN 10000

int big_array[BSS_LEN];

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

int main() {

    for(int i = 0; i < BSS_LEN; i ++)
        big_array[i] = i;


    char str[3];
    str[0] = 'A';
    str[1] = '\n';
    str[2] = 0;

    printf("PID: ");
    printf(itoa(syscall_process_get_pid(), 10));

    int iterations = 0;

    for(iterations = 0; iterations < 10; iterations ++) {
        for(int i = 0; i < 100000000; i ++){
			asm volatile("nop");
		}

        str[0]++;

        printf("Hello from program: ");
        printf(str);
    }

    return 0;
}