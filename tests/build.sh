ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding -I../sdk/libc/include -I../sdk/libkairax/include"
LD_ARGS="-melf_x86_64 --dynamic-linker=/loader.elf -z noexecstack"

mkdir -p obj
mkdir -p bin

gcc $ARGS sysc.c -o obj/sysc.o
ld $LD_ARGS -o bin/sysc.a obj/sysc.o ../sdk/crt/entry.o -L../sdk/libc/ -L../sdk/libkairax/ -lc -lerrno -lkairax

gcc $ARGS sysn.c -o obj/sysn.o
ld $LD_ARGS -o bin/sysn.a obj/sysn.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS floattest.c -o obj/floattest.o
ld $LD_ARGS -o bin/floattest.a obj/floattest.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS fs.c -o obj/fs.o
ld $LD_ARGS -o bin/fs.a obj/fs.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS pipe.c -o obj/pipe.o
ld $LD_ARGS -o bin/pipe.a obj/pipe.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS mem.c -o obj/mem.o
ld $LD_ARGS -o bin/mem.a obj/mem.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS assert.c -o obj/assert.o
ld $LD_ARGS -o bin/assert.a obj/assert.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS unixsock.c -o obj/unixsock.o
ld $LD_ARGS -o bin/unixsock.a obj/unixsock.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS fork.c -o obj/fork.o
ld $LD_ARGS -o bin/fork.a obj/fork.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS qemunet.c -o obj/qemunet.o
ld $LD_ARGS -o bin/qemunet.a obj/qemunet.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS udpsrv.c -o obj/udpsrv.o
ld $LD_ARGS -o bin/udpsrv.a obj/udpsrv.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS tcpcl.c -o obj/tcpcl.o
ld $LD_ARGS -o bin/tcpcl.a obj/tcpcl.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS udpcl.c -o obj/udpcl.o
ld $LD_ARGS -o bin/udpcl.a obj/udpcl.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS rterm.c -o obj/rterm.o
ld $LD_ARGS -o bin/rterm.a obj/rterm.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS bench.c -o obj/bench.o
ld $LD_ARGS -o bin/bench.a obj/bench.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS load.c -o obj/load.o
ld $LD_ARGS -o bin/load.a obj/load.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS getopt.c -o obj/getopt.o
ld $LD_ARGS -o bin/getopt.a obj/getopt.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS poll.c -o obj/poll.o
ld $LD_ARGS -o bin/poll.a obj/poll.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno

gcc $ARGS shm.c -o obj/shm.o
ld $LD_ARGS -o bin/shm.a obj/shm.o ../sdk/crt/entry.o -L../sdk/libc/ -lc -lerrno