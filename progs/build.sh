ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding"

gcc $ARGS sysc.c -o sysc.o
ld -melf_x86_64 -o sysc.a sysc.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc

gcc $ARGS sysn.c -o sysn.o
ld -melf_x86_64 -o sysn.a sysn.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc

cd ls
make
cd ..

gcc $ARGS echo.c -o echo.o
ld -melf_x86_64 -o echo.a echo.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc

gcc $ARGS copy.c -o copy.o
ld -melf_x86_64 -o copy.a copy.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc

gcc $ARGS mkdir.c -o mkdir.o
ld -melf_x86_64 -o mkdir.a mkdir.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc

gcc $ARGS cat.c -o cat.o
ld -melf_x86_64 -o cat.a cat.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc

gcc $ARGS floattest.c -o floattest.o
ld -melf_x86_64 -o floattest.a floattest.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc

gcc $ARGS shared_test.c -o shared_test.o
ld -melf_x86_64 -o shared_test.a shared_test.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc_dyn -lerrno

gcc $ARGS chmod.c -o chmod.o
ld -melf_x86_64 -o chmod.a chmod.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc