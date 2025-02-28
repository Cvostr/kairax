ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include -I../../sdk/libkairax/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: ln.o
	ld $(LDARGS) -o ln.a ln.o ../../sdk/crt/entry.o -L../../sdk/libc/ -L../../sdk/libkairax/ -lc -lerrno -lkairax

ln.o: ln.c
	$(COMPILER) $(ARGS) ln.c -o ln.o