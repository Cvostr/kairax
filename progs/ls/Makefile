ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: ls.o
	ld $(LDARGS) -o ls ls.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

ls.o: ls.c
	$(COMPILER) $(ARGS) ls.c -o ls.o