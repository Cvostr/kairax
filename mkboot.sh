#!/bin/bash

# build kernel
cd kernel
sh build.sh
cd ..

# build modules
cd modules/base
sh build.sh
cd ../..

# copy kernel
cp kernel/kernel.bin isofiles/boot/
# copy modules
cp modules/base/nvme/nvme.ko isofiles/boot/

grub-mkrescue -o kairax.iso isofiles