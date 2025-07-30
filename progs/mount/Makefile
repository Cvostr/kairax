ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: mount.o
	ld $(LDARGS) -o mount.a mount.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

mount.o: mount.c
	$(COMPILER) $(ARGS) mount.c -o mount.o