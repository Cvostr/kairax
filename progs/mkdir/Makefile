ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: mkdir.o
	ld $(LDARGS) -o mkdir.a mkdir.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

mkdir.o: mkdir.c
	$(COMPILER) $(ARGS) mkdir.c -o mkdir.o