ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding"
gcc $ARGS main.c -o main.o
ld -melf_x86_64 -o main.a main.o