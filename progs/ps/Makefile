ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf
COMPILER=gcc

all: ps.o
	ld $(LDARGS) -o ps.a ps.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

ps.o: ps.c
	$(COMPILER) $(ARGS) ps.c -o ps.o