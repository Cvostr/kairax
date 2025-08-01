ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: readlink.o
	ld $(LDARGS) -o readlink.a readlink.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

readlink.o: readlink.c
	$(COMPILER) $(ARGS) readlink.c -o readlink.o