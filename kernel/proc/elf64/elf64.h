#ifndef _ELF64_H
#define _ELF64_H

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

typedef struct PACKED {
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
} elf_header_t;

typedef struct PACKED {
    uint32_t    type;
    uint32_t    flags;
    uint64_t    p_offset;   //Смещение до данных
    uint64_t    v_addr;     //По этому адресу разместить данные
    uint64_t    undefined;
    uint64_t    p_filesz;   //Размер сегмента в файле
    uint64_t    p_memsz;    //Размер сегмента в памяти
    uint64_t    alignment;
} elf_program_header_entry_t; 

typedef struct PACKED {
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
} elf_section_header_entry_t;

typedef struct {
	uint32_t	    name;
	unsigned char	info;
	unsigned char	other;
	uint16_t	    shndx;
	uint64_t	    value;
	uint64_t	    size;
} elf_symbol_t;

struct process;

#define ELF_DT_NEEDED       1
#define ELF_DT_PLTGOT       3
#define ELF_DT_STRTAB       5
#define ELF_DT_SYMTAB       6
#define ELF_DT_RELA         7

typedef struct PACKED {
    int64_t tag;

    union {
        uint64_t val;
        uint64_t addr;
    } d_un;

} elf_dynamic_t;

int elf_check_signature(elf_header_t* elf_header);

elf_section_header_entry_t* elf_get_section_entry(char* image, uint32_t section_index);

elf_program_header_entry_t* elf_get_program_entry(char* image, uint32_t program_index);

char* elf_get_string_at(char* image, uint32_t string_index);

typedef struct {
    elf_section_header_entry_t* dynamic_ptr;
    elf_section_header_entry_t* dynsym_ptr;
    elf_section_header_entry_t* dynstr_ptr;
    elf_section_header_entry_t* rela_plt_ptr;
    elf_section_header_entry_t* tbss_ptr;
    elf_section_header_entry_t* tdata_ptr;
    elf_section_header_entry_t* comment_ptr;
} elf_sections_ptr_t;

void elf_read_sections(char* image, elf_sections_ptr_t* sections_struct);

int elf_load_process(struct process* process, char* image);

#endif