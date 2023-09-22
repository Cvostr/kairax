ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack

all: copy.o
	ld $(LDARGS) -o copy.a copy.o ../../sdk/sys/syscalls.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc_stat

copy.o: copy.c
	$(COMPILER) $(ARGS) copy.c -o copy.o