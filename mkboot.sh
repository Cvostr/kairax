cd kernel
sh build.sh
cd ..

cp kernel/kernel.bin isofiles/boot/

grub-mkrescue -o os.iso isofiles