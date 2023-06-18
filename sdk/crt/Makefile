ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding
COMPILER=gcc
ASM=nasm

all: entry.o

clean:
	rm -rf *.o

entry.o: entry.asm
	$(ASM) -felf64 entry.asm -o entry.o