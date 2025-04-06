ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf

all: clear.o
	ld $(LDARGS) -o clear.a clear.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

clear.o: clear.c
	$(COMPILER) $(ARGS) clear.c -o clear.o