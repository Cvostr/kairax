ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding
COMPILER=gcc
ASM=nasm

all: entry.o loader.o
	ld -n -o loader.elf -T ./link.ld entry.o loader.o ../../sdk/sys/syscalls.o

entry.o: entry.asm
	$(ASM) -felf64 entry.asm -o entry.o

loader.o: loader.c
	$(COMPILER) $(ARGS) loader.c -o loader.o