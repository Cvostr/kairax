ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf

all: rmdir.o
	ld $(LDARGS) -o rmdir.a rmdir.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

rmdir.o: rmdir.c
	$(COMPILER) $(ARGS) rmdir.c -o rmdir.o