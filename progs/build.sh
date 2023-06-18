ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding"

gcc $ARGS sysc.c -o sysc.o
ld -melf_x86_64 -o sysc.a sysc.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L./libc/ -lc

gcc $ARGS sysn.c -o sysn.o
ld -melf_x86_64 -o sysn.a sysn.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o