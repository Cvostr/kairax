ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -fPIC
COMPILER=gcc
ASM=nasm

all: entry.o loader.o linker.o
	ld -melf_x86_64 -o loader.elf entry.o loader.o linker.o ../../sdk/sys/syscalls.o

entry.o: entry.asm
	$(ASM) -felf64 entry.asm -o entry.o

loader.o: loader.c
	$(COMPILER) $(ARGS) loader.c -o loader.o

linker.o: linker.c
	$(COMPILER) $(ARGS) linker.c -o linker.o