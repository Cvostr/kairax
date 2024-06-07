ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding -I../sdk/libc/include -I../sdk/libkairax/include"
LD_ARGS="-melf_x86_64 --dynamic-linker=/loader.elf -z noexecstack"

gcc $ARGS sysc.c -o sysc.o
ld -melf_x86_64 $LD_ARGS -o sysc.a sysc.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS sysn.c -o sysn.o
ld -melf_x86_64 $LD_ARGS -o sysn.a sysn.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS floattest.c -o floattest.o
ld -melf_x86_64 $LD_ARGS -o floattest.a floattest.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS fs.c -o fs.o
ld -melf_x86_64 $LD_ARGS -o fs.a fs.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS pipe.c -o pipe.o
ld -melf_x86_64 $LD_ARGS -o pipe.a pipe.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS mem.c -o mem.o
ld -melf_x86_64 $LD_ARGS -o mem.a mem.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS assert.c -o assert.o
ld -melf_x86_64 $LD_ARGS -o assert.a assert.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno