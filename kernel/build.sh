#!/bin/bash

rm -rf bin
mkdir bin

x64_SRC=./arch/x86_64
STDC_PATH=./kairax
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
gcc $GCC_ARGS $x64_SRC/time/pit.c -o ./bin/pit.o
gcc $GCC_ARGS $x64_SRC/time/timer.c -o ./bin/timer_x64.o
gcc $GCC_ARGS $x64_SRC/interrupts/handle/handler.c -o ./bin/ints_handler.o
gcc $GCC_ARGS $x64_SRC/interrupts/handle/exceptions_handler.c -o ./bin/exceptions_handler.o
gcc $GCC_ARGS $x64_SRC/interrupts/pic.c -o ./bin/pic.o
gcc $GCC_ARGS $x64_SRC/interrupts/apic.c -o ./bin/apic.o
gcc $GCC_ARGS $x64_SRC/interrupts/ioapic.c -o ./bin/ioapic.o
gcc $GCC_ARGS $x64_SRC/memory/paging.c -o ./bin/paging_x64.o
gcc $GCC_ARGS $x64_SRC/memory/kernel_vmm.c -o ./bin/kernel_vmm.o
gcc $GCC_ARGS $x64_SRC/memory/iomem.c -o ./bin/iomem.o
gcc $GCC_ARGS $x64_SRC/dev/cmos/cmos.c -o ./bin/cmos.o
gcc $GCC_ARGS $x64_SRC/dev/keyboard/int_keyboard.c -o ./bin/int_keyboard.o
gcc $GCC_ARGS $x64_SRC/dev/acpi/acpi_dsdt.c -o ./bin/acpi_dsdt.o
gcc $GCC_ARGS $x64_SRC/dev/acpi/acpi.c -o ./bin/acpi.o
gcc $GCC_ARGS $x64_SRC/dev/acpi/acpi_madt.c -o ./bin/acpi_madt.o

gcc $GCC_ARGS $x64_SRC/proc/thread.c -o ./bin/thread_x64.o
gcc $GCC_ARGS $x64_SRC/proc/thread_scheduler.c -o ./bin/thread_scheduler_x64.o

#raxlib
gcc $GCC_ARGS $STDC_PATH/list/list.c -o ./bin/list.o
gcc $GCC_ARGS $STDC_PATH/vector/vector.c -o ./bin/vector.o
gcc $GCC_ARGS $STDC_PATH/guid/guid.c -o ./bin/guid.o

#generic kairax
gcc $GCC_ARGS $STDC_PATH/elf.c -o ./bin/elf.o
gcc $GCC_ARGS $STDC_PATH/string.c -o ./bin/stdc_string.o
gcc $GCC_ARGS $STDC_PATH/stdlib.c -o ./bin/stdc_stdlib.o
gcc $GCC_ARGS $STDC_PATH/ctype.c -o ./bin/stdc_ctype.o
gcc $GCC_ARGS $STDC_PATH/stdio.c -o ./bin/stdc_stdio.o
gcc $GCC_ARGS $STDC_PATH/time.c -o ./bin/time.o

#generic dev
gcc $GCC_ARGS dev/device_drivers.c -o ./bin/device_drivers.o
gcc $GCC_ARGS dev/device_man.c -o ./bin/device_man.o
gcc $GCC_ARGS dev/bus/pci/pci.c -o ./bin/pci.o

#generic drivers
gcc $GCC_ARGS drivers/storage/ahci/ahci.c -o ./bin/ahci.o
gcc $GCC_ARGS drivers/storage/ahci/ahci_port.c -o ./bin/ahci_port.o
gcc $GCC_ARGS drivers/storage/nvme/nvme.c -o ./bin/nvme.o
gcc $GCC_ARGS drivers/storage/devices/storage_devices.c -o ./bin/storage_devices.o
gcc $GCC_ARGS drivers/storage/partitions/storage_partitions.c -o ./bin/storage_partitions.o
gcc $GCC_ARGS drivers/storage/partitions/formats/gpt.c -o ./bin/gpt.o
gcc $GCC_ARGS drivers/video/video.c -o ./bin/video.o
gcc $GCC_ARGS drivers/tty/tty.c -o ./bin/tty.o

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
gcc $GCC_ARGS proc/elf_process_loader.c -o ./bin/elf_process_loader.o
gcc $GCC_ARGS proc/process.c -o ./bin/process.o
gcc $GCC_ARGS proc/process_list.c -o ./bin/process_list.o
gcc $GCC_ARGS proc/thread.c -o ./bin/thread.o
gcc $GCC_ARGS proc/thread_scheduler.c -o ./bin/thread_scheduler.o
gcc $GCC_ARGS proc/syscalls/syscalls.c -o ./bin/syscalls.o
gcc $GCC_ARGS proc/syscalls/syscalls_memory.c -o ./bin/syscalls_memory.o
gcc $GCC_ARGS proc/syscalls/syscalls_files.c -o ./bin/syscalls_files.o
gcc $GCC_ARGS proc/syscalls/syscall_wait.c -o ./bin/syscall_wait.o
gcc $GCC_ARGS proc/syscalls/syscalls_wd.c -o ./bin/syscalls_wd.o
gcc $GCC_ARGS proc/syscalls/syscalls_modules.c -o ./bin/syscalls_modules.o
gcc $GCC_ARGS proc/syscalls_table.c -o ./bin/syscalls_table.o
gcc $GCC_ARGS proc/timer.c -o ./bin/timer.o

#generic ipc
gcc $GCC_ARGS ipc/pipe.c -o ./bin/pipe.o

#generic sync
gcc $GCC_ARGS sync/semaphore.c -o ./bin/semaphore.o
gcc $GCC_ARGS sync/spinlock.c -o ./bin/spinlock.o

#generic memory
gcc $GCC_ARGS mem/paging.c -o ./bin/paging.o
gcc $GCC_ARGS mem/kheap.c -o ./bin/kheap.o
gcc $GCC_ARGS mem/pmm.c -o ./bin/pmm.o

#generic modules
gcc $GCC_ARGS mod/module_loader.c -o ./bin/module_loader.o
gcc $GCC_ARGS mod/function_table.c -o ./bin/function_table.o
gcc $GCC_ARGS mod/module_stor.c -o ./bin/module_stor.o

gcc $GCC_ARGS misc/bootshell/bootshell.c -o ./bin/bootshell.o
gcc $GCC_ARGS misc/bootshell/bootshell_cmdproc.c -o ./bin/bootshell_cmdproc.o
gcc $GCC_ARGS misc/kterm/kterm.c -o ./bin/kterm.o
gcc $GCC_ARGS misc/kterm/kterm_session.c -o ./bin/kterm_session.o
gcc $GCC_ARGS misc/kterm/vgaterm.c -o ./bin/vgaterm.o

ld -n -o kernel.bin -T ./arch/x86_64/link.ld bin/*.o
