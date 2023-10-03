ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: chmod.o
	ld $(LDARGS) -o chmod.a chmod.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

chmod.o: chmod.c
	$(COMPILER) $(ARGS) chmod.c -o chmod.o