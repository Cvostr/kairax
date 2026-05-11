ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: nc.o
	ld $(LDARGS) -o nc.a nc.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

nc.o: nc.c
	$(COMPILER) $(ARGS) nc.c -o nc.o