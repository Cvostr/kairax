ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf

all: rm.o
	ld $(LDARGS) -o rm.a rm.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

rm.o: rm.c
	$(COMPILER) $(ARGS) rm.c -o rm.o