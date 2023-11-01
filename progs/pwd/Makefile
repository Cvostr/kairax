ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf

all: pwd.o
	ld $(LDARGS) -o pwd.a pwd.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

pwd.o: pwd.c
	$(COMPILER) $(ARGS) pwd.c -o pwd.o