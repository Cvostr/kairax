#ifndef _ELF_H
#define _ELF_H

#include <inttypes.h>

typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

typedef uint32_t Elf32_Word;
typedef	int32_t  Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef	int32_t  Elf64_Sword;

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
} __attribute__((packed));

// ------RELOCATIONS ---------
struct elf_rel {
    void*       offset;
    uint64_t    info;
} __attribute__((packed));

struct elf_rela {
    void*       offset;
    uint64_t    info;
    int64_t     addend;
} __attribute__((packed));

//------DYNAMIC SECTION STUFF
#define ELF_DT_NEEDED       1
#define ELF_DT_PLTGOT       3
#define ELF_DT_STRTAB       5
#define ELF_DT_SYMTAB       6
#define ELF_DT_RELA         7
#define ELF_DT_RELASZ       8
#define ELF_DT_STRSZ        10

// EMT64/AMD64 Relocation types 
#define R_X86_64_COPY       5
#define R_X86_64_JUMP_SLOT  7
#define R_X86_64_RELATIVE   8
#define R_X86_64_GLOB_DAT   6

// ST_BIND values
#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2
#define STB_NUM         3

// ST_TYPE
#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3
#define STT_FILE        4

// A_TYPE
#define AT_NULL         0               
#define AT_IGNORE       1
#define AT_EXECFD       2
#define AT_PHDR         3
#define AT_PHENT        4
#define AT_PHNUM        5
#define AT_PAGESZ       6
#define AT_BASE         7
#define AT_FLAGS        8
#define AT_ENTRY        9

// Relocation macros
#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)

#define ELF64_SYM_BIND(i) ((i) >> 4)
#define ELF64_SYM_TYPE(i) ((i) & 0xF)

#define SHN_UNDEF       0

#endif