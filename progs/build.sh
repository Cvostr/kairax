ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding -I../sdk/libc/include -I../sdk/libkairax/include"
LD_ARGS="-melf_x86_64 --dynamic-linker=/loader.elf -z noexecstack"

gcc $ARGS sysc.c -o sysc.o
ld -melf_x86_64 -o sysc.a sysc.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc_stat

gcc $ARGS sysn.c -o sysn.o
ld -melf_x86_64 -o sysn.a sysn.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc_stat

cd ls
make
cd ..

gcc $ARGS echo.c -o echo.o
ld -melf_x86_64 -o echo.a echo.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc_stat

cd mkdir
make
cd ..

cd cat
make 
cd ..

gcc $ARGS floattest.c -o floattest.o
ld -melf_x86_64 -o floattest.a floattest.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc_stat

gcc $ARGS shared_test.c -o shared_test.o
ld $LD_ARGS -o shared-test.a shared_test.o ../sdk/sys/syscalls.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno -L../sdk/libkairax/ -lkairax

cd chmod
make
cd ..

cd copy
make
cd ..

cd dynloader
make
cd ..