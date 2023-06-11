ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding"

cd ./crt/
sh build.sh
cd ..

nasm -felf64 syscalls.asm -o syscalls.ob

gcc $ARGS sysc.c -o sysc.ob
ld -melf_x86_64 -o sysc.a sysc.ob syscalls.ob ./crt/entry.ob