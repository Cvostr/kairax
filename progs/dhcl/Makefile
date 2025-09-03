ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include -I../../sdk/libkairax/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: dhcl.o
	ld $(LDARGS) -o dhcl.a dhcl.o ../../sdk/crt/entry.o -L../../sdk/libc/ -L../../sdk/libkairax/ -lc -lerrno -lkairax

dhcl.o: dhcl.c
	$(COMPILER) $(ARGS) dhcl.c -o dhcl.o