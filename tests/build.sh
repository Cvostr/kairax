ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding -I../sdk/libc/include -I../sdk/libkairax/include"
LD_ARGS="-melf_x86_64 --dynamic-linker=/loader.elf -z noexecstack"

gcc $ARGS sysc.c -o sysc.o
ld $LD_ARGS -o sysc.a sysc.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS sysn.c -o sysn.o
ld $LD_ARGS -o sysn.a sysn.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS floattest.c -o floattest.o
ld $LD_ARGS -o floattest.a floattest.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS fs.c -o fs.o
ld $LD_ARGS -o fs.a fs.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS pipe.c -o pipe.o
ld $LD_ARGS -o pipe.a pipe.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS mem.c -o mem.o
ld $LD_ARGS -o mem.a mem.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS assert.c -o assert.o
ld $LD_ARGS -o assert.a assert.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS unixsock.c -o unixsock.o
ld $LD_ARGS -o unixsock.a unixsock.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS fork.c -o fork.o
ld $LD_ARGS -o fork.a fork.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS qemunet.c -o qemunet.o
ld $LD_ARGS -o qemunet.a qemunet.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno