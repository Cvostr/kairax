ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: realpath.o
	ld $(LDARGS) -o realpath.a realpath.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

realpath.o: realpath.c
	$(COMPILER) $(ARGS) realpath.c -o realpath.o