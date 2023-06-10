extern void printf(char*);

int main() {

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