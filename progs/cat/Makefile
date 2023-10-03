ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: cat.o
	ld $(LDARGS) -o cat.a cat.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

cat.o: cat.c
	$(COMPILER) $(ARGS) cat.c -o cat.o