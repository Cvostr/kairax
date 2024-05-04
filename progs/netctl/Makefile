ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include -I../../sdk/libkairax/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: main.o
	ld $(LDARGS) -o netctl.a main.o ../../sdk/crt/entry.o -L../../sdk/libc/ -L../../sdk/libkairax/ -lc -lerrno -lkairax

main.o: main.c
	$(COMPILER) $(ARGS) main.c -o main.o