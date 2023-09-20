CFLAGS=-nostdlib -m64 -c -nostdinc -ffreestanding -fPIC -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack -pie
CC=gcc
ASM=nasm

all: entry.o loader.o linker.o elf.o string.o
	ld $(LDARGS) -o loader.elf entry.o elf.o string.o loader.o linker.o ../../sdk/sys/syscalls.o

entry.o: entry.asm
	$(ASM) -felf64 entry.asm -o entry.o

elf.o: elf.c
	$(CC) $(CFLAGS) elf.c -o elf.o

string.o: string.c
	$(CC) $(CFLAGS) string.c -o string.o

loader.o: loader.c
	$(CC) $(CFLAGS) loader.c -o loader.o

linker.o: linker.c
	$(CC) $(CFLAGS) linker.c -o linker.o