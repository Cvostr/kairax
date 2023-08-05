ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding"

gcc $ARGS sysc.c -o sysc.o
ld -melf_x86_64 -o sysc.a sysc.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc

gcc $ARGS sysn.c -o sysn.o
ld -melf_x86_64 -o sysn.a sysn.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc

gcc $ARGS ls.c -o ls.o
ld -melf_x86_64 -o ls.a ls.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc

gcc $ARGS echo.c -o echo.o
ld -melf_x86_64 -o echo.a echo.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc