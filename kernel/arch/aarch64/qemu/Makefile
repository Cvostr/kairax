CC=aarch64-linux-gnu-gcc
AS=aarch64-linux-gnu-as
LD=aarch64-linux-gnu-ld

CFLAGS=-ffreestanding -O1 -Wall -Wextra
ARCH_KERNEL_CFLAGS = -z max-page-size=0x1000 -nostdlib -mgeneral-regs-only -mno-outline-atomics -ffixed-x18 --sysroot=base

all: kernel

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

start.o: start.S
	$(AS) start.S -o start.o

kernel: main.o start.o
	$(CC) -T link.ld -o kernel.elf -ffreestanding -O1 -nostdlib main.o start.o -lgcc 

clean:
	rm *.o