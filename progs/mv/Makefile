ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: mv.o
	ld $(LDARGS) -o mv.a mv.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

mv.o: mv.c
	$(COMPILER) $(ARGS) mv.c -o mv.o