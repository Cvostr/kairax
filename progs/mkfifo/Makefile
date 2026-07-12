ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: mkfifo.o
	ld $(LDARGS) -o mkfifo.a mkfifo.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

mkfifo.o: mkfifo.c
	$(COMPILER) $(ARGS) mkfifo.c -o mkfifo.o