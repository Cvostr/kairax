ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include -I../../sdk/libkairax/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: modctl.o
	ld $(LDARGS) -o modctl.a modctl.o ../../sdk/crt/entry.o -L../../sdk/libc/ -L../../sdk/libkairax/ -lc -lerrno -lkairax

modctl.o: modctl.c
	$(COMPILER) $(ARGS) modctl.c -o modctl.o