ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack
COMPILER=gcc

all: chmod.o
	ld $(LDARGS) -o chmod.a chmod.o ../../sdk/sys/syscalls.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc_stat

chmod.o: chmod.c
	$(COMPILER) $(ARGS) chmod.c -o chmod.o