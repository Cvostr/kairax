ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf

PROGNAME=env.a

all: main.o
	ld $(LDARGS) -o $(PROGNAME) main.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

main.o: main.c
	$(COMPILER) $(ARGS) main.c -o main.o