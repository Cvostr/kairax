char* buffer = (char*)0xFFFF8880000B8000;

char sym = 'A';

int main() {

    while(1) {
        for(int i = 0; i < 100000000; i ++){
			asm volatile("nop");
		}

        //sym++;
        asm volatile("syscall");
        //*(buffer + 20) = sym;
    }

    return 0;
}