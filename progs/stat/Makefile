ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf

all: stat.o
	ld $(LDARGS) -o stat.a stat.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

stat.o: stat.c
	$(COMPILER) $(ARGS) stat.c -o stat.o