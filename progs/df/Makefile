ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: df.o
	ld $(LDARGS) -o df.a df.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

df.o: df.c
	$(COMPILER) $(ARGS) df.c -o df.o