ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
LDARGS=-melf_x86_64 -z noexecstack
COMPILER=gcc

all: cat.o
	ld $(LDARGS) -o cat.a cat.o ../../sdk/sys/syscalls.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc_stat

cat.o: cat.c
	$(COMPILER) $(ARGS) cat.c -o cat.o