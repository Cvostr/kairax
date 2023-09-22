ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack
COMPILER=gcc

all: mkdir.o
	ld $(LDARGS) -o mkdir.a mkdir.o ../../sdk/sys/syscalls.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc_stat

mkdir.o: mkdir.c
	$(COMPILER) $(ARGS) mkdir.c -o mkdir.o