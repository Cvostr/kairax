#!/bin/bash

rm -rf bin
mkdir bin

x64_SRC=./arch/x86_64
GCC_ARGS="-nostdlib -m64 -c -nostdinc -I$x64_SRC/ -I$x64_SRC/base/stdc -I$x64_SRC/base -ffreestanding -mcmodel=large -mno-red-zone -fno-stack-protector -fno-omit-frame-pointer -nostartfiles -static"
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
gcc $GCC_ARGS $x64_SRC/dev/pci/pci.c -o ./bin/pci.o
gcc $GCC_ARGS $x64_SRC/dev/ahci/ahci.c -o ./bin/ahci.o

nasm $NASM_ARGS $x64_SRC/proc/context_switch.asm -o ./bin/context_switch.o
gcc $GCC_ARGS $x64_SRC/proc/process.c -o ./bin/process.o
gcc $GCC_ARGS $x64_SRC/proc/thread.c -o ./bin/thread.o
gcc $GCC_ARGS $x64_SRC/proc/thread_scheduler.c -o ./bin/thread_scheduler.o

gcc $GCC_ARGS $x64_SRC/base/x86-console/x86-console.c -o ./bin/x86-console.o

gcc $GCC_ARGS $x64_SRC/base/list/list.c -o ./bin/list.o
#stdc
gcc $GCC_ARGS $x64_SRC/base/stdc/stdlib.c -o ./bin/stdc_stdlib.o
gcc $GCC_ARGS $x64_SRC/base/stdc/string.c -o ./bin/stdc_string.o
gcc $GCC_ARGS $x64_SRC/base/stdc/stdio.c -o ./bin/stdc_stdio.o


ld -n -o kernel.bin -T ./arch/x86_64/link.ld bin/*.o
