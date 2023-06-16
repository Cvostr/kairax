ARGS="-nostdlib -m64 -c -nostdinc -ffreestanding"

cd ./crt/
sh build.sh
cd ..

cd ./sdk/
sh build.sh
cd ..

gcc $ARGS sysc.c -o sysc.ob
ld -melf_x86_64 -o sysc.a sysc.ob ./sdk/syscalls.ob ./crt/entry.ob -L./libc/ -lc

gcc $ARGS sysn.c -o sysn.ob
ld -melf_x86_64 -o sysn.a sysn.ob ./sdk/syscalls.ob ./crt/entry.ob