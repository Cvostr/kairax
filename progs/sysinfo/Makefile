ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include -I../../sdk/libkairax/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: sysinfo.o
	ld $(LDARGS) -o sysinfo.a sysinfo.o ../../sdk/crt/entry.o -L../../sdk/libc/ -L../../sdk/libkairax/ -lc -lerrno -lkairax

sysinfo.o: sysinfo.c
	$(COMPILER) $(ARGS) sysinfo.c -o sysinfo.o