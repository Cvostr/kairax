ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf

all: touch.o
	ld $(LDARGS) -o touch.a touch.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

touch.o: touch.c
	$(COMPILER) $(ARGS) touch.c -o touch.o