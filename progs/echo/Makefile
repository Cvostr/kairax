ARGS=-nostdlib -m64 -c -nostdinc -ffreestanding -I../../sdk/libc/include
COMPILER=gcc
LDARGS=-melf_x86_64 -z noexecstack --dynamic-linker=/loader.elf

all: echo.o
	ld $(LDARGS) -o echo.a echo.o ../../sdk/crt/entry.o -L../../sdk/libc/ -lc -lerrno

echo.o: echo.c
	$(COMPILER) $(ARGS) echo.c -o echo.o