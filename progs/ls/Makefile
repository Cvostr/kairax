ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding
COMPILER=gcc

all: ls.o
	ld -melf_x86_64 -o ls.a ls.o ../../sdk/sys/syscalls.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc

ls.o: ls.c
	$(COMPILER) $(ARGS) ls.c -o ls.o