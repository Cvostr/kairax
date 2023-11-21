ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding -I../sdk/libc/include -I../sdk/libkairax/include"
LD_ARGS="-melf_x86_64 --dynamic-linker=/loader.elf -z noexecstack"

gcc $ARGS sysc.c -o sysc.o
ld -melf_x86_64 $LD_ARGS -o sysc.a sysc.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS sysn.c -o sysn.o
ld -melf_x86_64 $LD_ARGS -o sysn.a sysn.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

cd ls
make
cd ..

cd echo
make
cd ..

cd mkdir
make
cd ..

cd cat
make 
cd ..

cd date
make
cd ..

cd pwd
make
cd ..

gcc $ARGS floattest.c -o floattest.o
ld -melf_x86_64 $LD_ARGS -o floattest.a floattest.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

cd chmod
make
cd ..

cd copy
make
cd ..

cd mv
make
cd ..

cd dynloader
make
cd ..