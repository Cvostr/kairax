ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding"

nasm -felf64 syscalls.asm -o syscalls.o

gcc $ARGS main.c -o main.o
ld -melf_x86_64 -o main.a main.o

gcc $ARGS sysc.c -o sysc.o
ld -melf_x86_64 -o sysc.a sysc.o syscalls.o