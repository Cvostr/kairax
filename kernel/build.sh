#!/bin/bash

rm -rf bin
mkdir bin

x64_SRC=./arch/x86_64
RAXLIB_PATH=./raxlib
GCC_ARGS="-nostdlib -m64 -c -nostdinc -I$RAXLIB_PATH/ -I./ -I$x64_SRC/ -I$x64_SRC/base/stdc -I$x64_SRC/base -ffreestanding -mcmodel=large -mno-red-zone -fno-stack-protector -fno-omit-frame-pointer -nostartfiles -static"
NASM_ARGS="-felf64 -i$x64_SRC"

nasm $NASM_ARGS $x64_SRC/boot/start.asm -o ./bin/start.o
nasm $NASM_ARGS $x64_SRC/boot/multiboot2_header.asm -o ./bin/multiboot2_header.o
nasm $NASM_ARGS $x64_SRC/boot/gdt_table.asm -o ./bin/gdt_table.o
nasm $NASM_ARGS $x64_SRC/boot/tss_table.asm -o ./bin/tss_table.o
nasm $NASM_ARGS $x64_SRC/boot/x64.asm -o ./bin/x64.o
nasm $NASM_ARGS $x64_SRC/interrupts/interrupts.asm -o ./bin/interrupts.o

gcc $GCC_ARGS $x64_SRC/boot/multiboot.c -o ./bin/multiboot.o

gcc $GCC_ARGS $x64_SRC/kernel.c -o ./bin/kernel.o
gcc $GCC_ARGS $x64_SRC/interrupts/idt.c -o ./bin/idt.o
gcc $GCC_ARGS $x64_SRC/interrupts/handle/handler.c -o ./bin/ints_handler.o
gcc $GCC_ARGS $x64_SRC/interrupts/handle/exceptions_handler.c -o ./bin/exceptions_handler.o
gcc $GCC_ARGS $x64_SRC/interrupts/pic.c -o ./bin/pic.o
gcc $GCC_ARGS $x64_SRC/memory/pmm.c -o ./bin/pmm.o
gcc $GCC_ARGS $x64_SRC/memory/paging.c -o ./bin/paging.o
gcc $GCC_ARGS $x64_SRC/dev/cmos/cmos.c -o ./bin/cmos.o
gcc $GCC_ARGS $x64_SRC/dev/keyboard/int_keyboard.c -o ./bin/int_keyboard.o
gcc $GCC_ARGS $x64_SRC/dev/acpi/acpi.c -o ./bin/acpi.o

gcc $GCC_ARGS $x64_SRC/proc/process.c -o ./bin/process.o
gcc $GCC_ARGS $x64_SRC/proc/thread.c -o ./bin/thread.o
gcc $GCC_ARGS $x64_SRC/proc/thread_scheduler.c -o ./bin/thread_scheduler.o

gcc $GCC_ARGS $x64_SRC/base/x86-console/x86-console.c -o ./bin/x86-console.o
#stdc
gcc $GCC_ARGS $x64_SRC/base/stdc/stdlib.c -o ./bin/stdc_stdlib.o
gcc $GCC_ARGS $x64_SRC/base/stdc/string.c -o ./bin/stdc_string.o
gcc $GCC_ARGS $x64_SRC/base/stdc/stdio.c -o ./bin/stdc_stdio.o

#raxlib
gcc $GCC_ARGS $RAXLIB_PATH/list/list.c -o ./bin/list.o

#generic bus
gcc $GCC_ARGS bus/pci/pci.c -o ./bin/pci.o

#generic drivers
gcc $GCC_ARGS drivers/storage/ahci/ahci.c -o ./bin/ahci.o
gcc $GCC_ARGS drivers/storage/ahci/ahci_port.c -o ./bin/ahci_port.o
gcc $GCC_ARGS drivers/storage/nvme/nvme.c -o ./bin/nvme.o

#generic fs
gcc $GCC_ARGS fs/ext2/ext2.c -o ./bin/ext2.o
gcc $GCC_ARGS fs/vfs/vfs.c -o ./bin/vfs.o

#generic sync
gcc $GCC_ARGS sync/spinlock.c -o ./bin/spinlock.o

gcc $GCC_ARGS mem/kheap.c -o ./bin/kheap.o

ld -n -o kernel.bin -T ./arch/x86_64/link.ld bin/*.o
