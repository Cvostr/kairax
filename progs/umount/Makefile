ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: umount.o
	ld $(LDARGS) -o umount.a umount.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

umount.o: umount.c
	$(COMPILER) $(ARGS) umount.c -o umount.o