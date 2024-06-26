#ifndef _KX_ELF_H
#define _KX_ELF_H

#include "types.h"

#define ELF_BITS_32         1
#define ELF_BITS_64         2

#define ELF_BYTE_ORDER_LE   1
#define ELF_BYTE_ORDER_BE   2

#define ELF_ABI_SYSTEM_V    0

#define ELF_TYPE_RELOCATABLE 1
#define ELF_TYPE_EXECUTABLE  2
#define ELF_TYPE_SHARED      3
#define ELF_TYPE_CORE        4

#define ELF_ARCH_X86        3
#define ELF_ARCH_X86_64     0x3E
#define ELF_ARCH_AARCH64    0xB7
#define ELF_ARCH_RISCV      0xF3

#define ELF_SEGMENT_TYPE_IGNORED    0
#define ELF_SEGMENT_TYPE_LOAD       1
#define ELF_SEGMENT_TYPE_DYNAMIC    2
#define ELF_SEGMENT_TYPE_INTERP     3
#define ELF_SEGMENT_TYPE_NOTE       4

#define ELF_SEGMENT_FLAG_EXEC       1
#define ELF_SEGMENT_FLAG_WRITE      2
#define ELF_SEGMENT_FLAG_READ       4

#define ELF_SECTION_TYPE_NULL       0
#define ELF_SECTION_TYPE_PROGBITS   1
#define ELF_SECTION_TYPE_SYMTAB     2
#define ELF_SECTION_TYPE_STRTAB     3
#define ELF_SECTION_TYPE_RELA       4
#define ELF_SECTION_TYPE_HASH       5
#define ELF_SECTION_TYPE_DYNAMIC    6
#define ELF_SECTION_TYPE_NOTE       7
#define ELF_SECTION_TYPE_NOBITS     8
#define ELF_SECTION_TYPE_REL        9
#define ELF_SECTION_TYPE_SHLIB      10
#define ELF_SECTION_TYPE_DYNSYM     11
#define ELF_SECTION_TYPE_LOPROC     0x70000000
#define ELF_SECTION_TYPE_HIPROC     0x7fffffff
#define ELF_SECTION_TYPE_LOUSER     0x80000000
#define ELF_SECTION_TYPE_HIUSER     0xffffffff

#define ELF_SECTION_FLAG_WRITE      0x1
#define ELF_SECTION_FLAG_ALLOC      0x2
#define ELF_SECTION_FLAG_EXECINSRTR 0x4
#define ELF_SECTION_FLAG_TLS        0x400
#define ELF_SECTION_FLAG_MASKPROC   0xF0000000   

#define ELF_DT_NEEDED       1
#define ELF_DT_PLTGOT       3
#define ELF_DT_STRTAB       5
#define ELF_DT_SYMTAB       6
#define ELF_DT_RELA         7

struct elf_header {
    char        header[4];
    uint8_t     bits;
    uint8_t     byte_order;
    uint8_t     elf_header_version;
    uint8_t     abi;
    uint64_t    unused;
    uint16_t    type;
    uint16_t    arch;
    uint32_t    elf_version;
    uint64_t    prog_entry_pos;
    uint64_t    prog_header_table_pos;
    uint64_t    section_header_table_pos;
    uint32_t    flags;
    uint16_t    header_size;
    uint16_t    prog_header_entry_size;
    uint16_t    prog_header_entries_num;
    uint16_t    section_header_entry_size;
    uint16_t    section_header_entries_num;
    uint16_t    section_names_index;
} PACKED;

struct elf_program_header_entry {
    uint32_t    type;
    uint32_t    flags;
    uint64_t    p_offset;   //Смещение до данных
    uint64_t    v_addr;     //По этому адресу разместить данные
    uint64_t    undefined;
    uint64_t    p_filesz;   //Размер сегмента в файле
    uint64_t    p_memsz;    //Размер сегмента в памяти
    uint64_t    alignment;
} PACKED; 

struct elf_section_header_entry {
    uint32_t    name_offset;
    uint32_t    type;
    uint64_t    flags;
    uintptr_t   addr;       // Адрес в виртуальной памяти
    uintptr_t   offset;     // Смещение от начала файла
    uint64_t    size;
    uint32_t    link;
    uint32_t    info;
    uint64_t    addralign;
    uint64_t    ent_size;
} PACKED;

struct elf_symbol {
	uint32_t	    name;
	unsigned char	info;
	unsigned char	other;
	uint16_t	    shndx;
	uint64_t	    value;
	uint64_t	    size;
} PACKED;

struct elf_rela {
    void*       offset;
    uint64_t    info;
    int64_t     addend;
} PACKED;

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)

#define R_X86_64_64		    1
#define R_X86_64_PC32		2
#define R_X86_64_GOT32		3
#define R_X86_64_PLT32		4

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2

#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3
#define STT_FILE        4

#define SHN_UNDEF       0

#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4

int elf_check_signature(struct elf_header* elf_header);

struct elf_section_header_entry* elf_get_section_entry(const char* image, uint32_t section_index);

struct elf_program_header_entry* elf_get_program_entry(char* image, uint32_t program_index);

char* elf_get_string_at(const char* image, uint32_t string_index);

#endif