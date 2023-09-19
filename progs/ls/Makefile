ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack
COMPILER=gcc

all: ls.o
	ld $(LDARGS) -o ls.a ls.o ../../sdk/sys/syscalls.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc_stat

ls.o: ls.c
	$(COMPILER) $(ARGS) ls.c -o ls.o