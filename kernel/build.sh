#!/bin/bash

rm -rf bin
mkdir bin

x64_SRC=./arch/x86_64
RAXLIB_PATH=./raxlib
STDC_PATH=./stdc
GCC_ARGS="-nostdlib -m64 -c -nostdinc -I$STDC_PATH -I$RAXLIB_PATH/ -I./ -I$x64_SRC/ -I$x64_SRC/base/stdc -I$x64_SRC/base -ffreestanding -mcmodel=kernel -fno-pic -mno-red-zone -fno-omit-frame-pointer -nostartfiles -static"
NASM_ARGS="-felf64 -i$x64_SRC"

nasm $NASM_ARGS $x64_SRC/boot/start.asm -o ./bin/start.o
nasm $NASM_ARGS $x64_SRC/boot/multiboot2_header.asm -o ./bin/multiboot2_header.o
nasm $NASM_ARGS $x64_SRC/boot/gdt_table.asm -o ./bin/gdt_table.o
nasm $NASM_ARGS $x64_SRC/boot/tss_table.asm -o ./bin/tss_table.o
nasm $NASM_ARGS $x64_SRC/boot/x64.asm -o ./bin/x64.o
nasm $NASM_ARGS $x64_SRC/interrupts/interrupts.asm -o ./bin/interrupts.o
nasm $NASM_ARGS $x64_SRC/proc/syscalls_entry.asm -o ./bin/syscalls_entry.o
nasm $NASM_ARGS $x64_SRC/proc/scheduler_entry.asm -o ./bin/scheduler_entry.o
nasm $NASM_ARGS $x64_SRC/cpu/smp_trampoline.asm -o ./bin/smp_trampoline.o

gcc $GCC_ARGS $x64_SRC/boot/multiboot.c -o ./bin/multiboot.o

gcc $GCC_ARGS $x64_SRC/cpu/msr.c -o ./bin/msr.o
gcc $GCC_ARGS $x64_SRC/cpu/gdt.c -o ./bin/gdt.o
gcc $GCC_ARGS $x64_SRC/cpu/smp.c -o ./bin/smp.o
gcc $GCC_ARGS $x64_SRC/cpu/cpuid.c -o ./bin/cpuid.o

gcc $GCC_ARGS $x64_SRC/kernel.c -o ./bin/kernel.o
gcc $GCC_ARGS $x64_SRC/interrupts/idt.c -o ./bin/idt.o
gcc $GCC_ARGS $x64_SRC/interrupts/pit.c -o ./bin/pit.o
gcc $GCC_ARGS $x64_SRC/interrupts/handle/handler.c -o ./bin/ints_handler.o
gcc $GCC_ARGS $x64_SRC/interrupts/handle/exceptions_handler.c -o ./bin/exceptions_handler.o
gcc $GCC_ARGS $x64_SRC/interrupts/pic.c -o ./bin/pic.o
gcc $GCC_ARGS $x64_SRC/interrupts/apic.c -o ./bin/apic.o
gcc $GCC_ARGS $x64_SRC/memory/paging.c -o ./bin/paging.o
gcc $GCC_ARGS $x64_SRC/memory/kernel_vmm.c -o ./bin/kernel_vmm.o
gcc $GCC_ARGS $x64_SRC/dev/cmos/cmos.c -o ./bin/cmos.o
gcc $GCC_ARGS $x64_SRC/dev/keyboard/int_keyboard.c -o ./bin/int_keyboard.o
gcc $GCC_ARGS $x64_SRC/dev/acpi/acpi_dsdt.c -o ./bin/acpi_dsdt.o
gcc $GCC_ARGS $x64_SRC/dev/acpi/acpi.c -o ./bin/acpi.o
gcc $GCC_ARGS $x64_SRC/dev/acpi/acpi_madt.c -o ./bin/acpi_madt.o
gcc $GCC_ARGS $x64_SRC/dev/b8-console/b8-console.c -o ./bin/b8-console.o

gcc $GCC_ARGS $x64_SRC/proc/process.c -o ./bin/process_x64.o
gcc $GCC_ARGS $x64_SRC/proc/thread.c -o ./bin/thread_x64.o
gcc $GCC_ARGS $x64_SRC/proc/thread_scheduler.c -o ./bin/thread_scheduler.o
gcc $GCC_ARGS $x64_SRC/proc/syscall_handle.c -o ./bin/syscall_handle.o
#stdc
gcc $GCC_ARGS $x64_SRC/base/stdc/stdio.c -o ./bin/stdc_stdio.o

#raxlib
gcc $GCC_ARGS $RAXLIB_PATH/list/list.c -o ./bin/list.o
gcc $GCC_ARGS $RAXLIB_PATH/guid/guid.c -o ./bin/guid.o

#generic stdc
gcc $GCC_ARGS $STDC_PATH/string.c -o ./bin/stdc_string.o
gcc $GCC_ARGS $STDC_PATH/stdlib.c -o ./bin/stdc_stdlib.o
gcc $GCC_ARGS $STDC_PATH/ctype.c -o ./bin/stdc_ctype.o

#generic bus
gcc $GCC_ARGS bus/pci/pci.c -o ./bin/pci.o

#generic drivers
gcc $GCC_ARGS drivers/storage/ata/ata.c -o ./bin/ata.o
gcc $GCC_ARGS drivers/storage/ahci/ahci.c -o ./bin/ahci.o
gcc $GCC_ARGS drivers/storage/ahci/ahci_port.c -o ./bin/ahci_port.o
gcc $GCC_ARGS drivers/storage/nvme/nvme.c -o ./bin/nvme.o
gcc $GCC_ARGS drivers/storage/devices/storage_devices.c -o ./bin/storage_devices.o
gcc $GCC_ARGS drivers/storage/partitions/storage_partitions.c -o ./bin/storage_partitions.o
gcc $GCC_ARGS drivers/storage/partitions/formats/gpt.c -o ./bin/gpt.o

#generic fs
gcc $GCC_ARGS fs/devfs/devfs.c -o ./bin/devfs.o
gcc $GCC_ARGS fs/ext2/ext2.c -o ./bin/ext2.o
gcc $GCC_ARGS fs/vfs/vfs.c -o ./bin/vfs.o
gcc $GCC_ARGS fs/vfs/filesystems.c -o ./bin/filesystems.o
gcc $GCC_ARGS fs/vfs/inode.c -o ./bin/inode.o
gcc $GCC_ARGS fs/vfs/dentry.c -o ./bin/dentry.o
gcc $GCC_ARGS fs/vfs/file.c -o ./bin/file.o
gcc $GCC_ARGS fs/vfs/superblock.c -o ./bin/superblock.o

#generic proc
gcc $GCC_ARGS proc/process.c -o ./bin/process.o
gcc $GCC_ARGS proc/thread.c -o ./bin/thread.o

#generic ipc
gcc $GCC_ARGS ipc/pipe.c -o ./bin/pipe.o

#generic sync
gcc $GCC_ARGS sync/spinlock.c -o ./bin/spinlock.o

#generic memory
gcc $GCC_ARGS mem/kheap.c -o ./bin/kheap.o
gcc $GCC_ARGS mem/pmm.c -o ./bin/pmm.o

#ELF
gcc $GCC_ARGS proc/elf64/elf64.c -o ./bin/elf64.o

gcc $GCC_ARGS misc/bootshell/bootshell.c -o ./bin/bootshell.o
gcc $GCC_ARGS misc/bootshell/bootshell_cmdproc.c -o ./bin/bootshell_cmdproc.o

ld -n -o kernel.bin -T ./arch/x86_64/link.ld bin/*.o
