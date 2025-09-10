ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf

all: stty.o
	ld $(LDARGS) -o stty.a stty.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

stty.o: stty.c
	$(COMPILER) $(ARGS) stty.c -o stty.o