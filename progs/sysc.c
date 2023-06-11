extern void printf(char*);

#define BSS_LEN 10000

int big_array[BSS_LEN];

int main() {

    for(int i = 0; i < BSS_LEN; i ++)
        big_array[i] = i;


    char str[3];
    str[1] = '\n';
    str[2] = 0;

    while(1) {
        for(int i = 0; i < 100000000; i ++){
			asm volatile("nop");
		}

        str[0]++;

        printf("Hello from program: ");
        printf(str);
    }

    return 0;
}