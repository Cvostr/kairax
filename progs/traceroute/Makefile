ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include -I../../sdk/libkairax/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: traceroute.o
	ld $(LDARGS) -o traceroute.a traceroute.o ../../sdk/crt/entry.o -L../../sdk/libc/ -L../../sdk/libkairax/ -lc -lerrno -lkairax

traceroute.o: traceroute.c
	$(COMPILER) $(ARGS) traceroute.c -o traceroute.o