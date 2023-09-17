ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -fPIC
COMPILER=gcc
ASM=nasm

all: entry.o loader.o linker.o elf.o string.o
	ld -melf_x86_64 -pie -o loader.elf entry.o elf.o string.o loader.o linker.o ../../sdk/sys/syscalls.o

entry.o: entry.asm
	$(ASM) -felf64 entry.asm -o entry.o

elf.o: elf.c
	$(COMPILER) $(ARGS) elf.c -o elf.o

string.o: string.c
	$(COMPILER) $(ARGS) string.c -o string.o

loader.o: loader.c
	$(COMPILER) $(ARGS) loader.c -o loader.o

linker.o: linker.c
	$(COMPILER) $(ARGS) linker.c -o linker.o