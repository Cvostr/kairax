ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding
COMPILER=gcc
ASM=nasm

all: syscalls.o

clean:
	rm -rf *.o

syscalls.o: syscalls.asm
	$(ASM) -felf64 syscalls.asm -o syscalls.o