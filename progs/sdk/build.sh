ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding"

nasm -felf64 syscalls.asm -o syscalls.ob