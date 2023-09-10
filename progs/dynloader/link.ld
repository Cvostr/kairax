ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT(kairax-x86_64.bin)

SECTIONS {
    . = 0;

    .text : {
        *(.text .text.*)
    }

    .data : {
        *(.data .data.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }

    .bss : {
        *(.bss .bss.*)
    }

    . = ALIGN(0x1000);
}