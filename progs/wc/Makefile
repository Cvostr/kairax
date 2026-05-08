ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: wc.o
	ld $(LDARGS) -o wc.a wc.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

wc.o: wc.c
	$(COMPILER) $(ARGS) wc.c -o wc.o