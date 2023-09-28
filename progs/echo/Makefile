ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack

all: echo.o
	ld $(LDARGS) -o echo.a echo.o ../../sdk/sys/syscalls.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc_stat

echo.o: echo.c
	$(COMPILER) $(ARGS) echo.c -o echo.o