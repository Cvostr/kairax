%define MAGIC_HEADER_MB2          0xE85250D6
%define PROTECTED_MODE            0x0
%define MULTIBOOT_HEADER_TAG_END  0x0
%define MAGIC_BOOTLOADER_MB2      0x36d76289

%define MULTIBOOT_HEADER_TAG_FRAMEBUFFER  5
%define MULTIBOOT_HEADER_TAG_OPTIONAL 1

bits 32

section .multiboot
align 8
mb_header:
    dd MAGIC_HEADER_MB2
    dd PROTECTED_MODE            ; architecture 0 (protected mode i386)
    dd mb_header_end - mb_header ; mb_header length
    ; checksum
    dd 0x100000000 - (MAGIC_HEADER_MB2 + PROTECTED_MODE + (mb_header_end - mb_header))

align 8
framebuffer_tag_start:  
    dw MULTIBOOT_HEADER_TAG_FRAMEBUFFER
    dw 0
    dd framebuffer_tag_end - framebuffer_tag_start
    dd 1152
    dd 864
    dd 32
framebuffer_tag_end:

align 8
    dw MULTIBOOT_HEADER_TAG_END ; type
    dw 0x0                      ; flags
    dd 0x8                      ; size
mb_header_end: