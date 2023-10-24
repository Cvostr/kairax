ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf

all: date.o
	ld $(LDARGS) -o date.a date.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

date.o: date.c
	$(COMPILER) $(ARGS) date.c -o date.o