#ifndef _ELF_H
#define _ELF_H

#include <sys/types.h>
#include <stdint.h>

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

struct elf_program_header_entry {
    uint32_t    type;
    uint32_t    flags;
    uint64_t    p_offset;   //Смещение до данных
    uint64_t    v_addr;     //По этому адресу разместить данные
    uint64_t    undefined;
    uint64_t    p_filesz;   //Размер сегмента в файле
    uint64_t    p_memsz;    //Размер сегмента в памяти
    uint64_t    alignment;
} __attribute__((packed)); 

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
} __attribute__((packed));

//------DYNAMIC SECTION STUFF
#define ELF_DT_NEEDED       1
#define ELF_DT_PLTGOT       3
#define ELF_DT_STRTAB       5
#define ELF_DT_SYMTAB       6
#define ELF_DT_RELA         7
#define ELF_DT_RELASZ       8
#define ELF_DT_STRSZ        10

struct elf_dynamic {
    int64_t tag;

    union {
        uint64_t val;
        uint64_t addr;
    } d_un;

} __attribute__((packed));

// -------SYMBOLS STUFF
struct elf_symbol {
	uint32_t	    name;
	unsigned char	info;
	unsigned char	other;
	uint16_t	    shndx;
	uint64_t	    value;
	uint64_t	    size;
}  __attribute__((packed));

// GOT PLT
struct got_plt {
    void* unused;
    void* arg;
    void* linker_ip;
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

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)

#define R_X86_64_COPY       5
#define R_X86_64_JUMP_SLOT  7
#define R_X86_64_RELATIVE   8
#define R_X86_64_GLOB_DAT   6

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

#define ELF_SEGMENT_TYPE_IGNORED    0
#define ELF_SEGMENT_TYPE_LOAD       1
#define ELF_SEGMENT_TYPE_DYNAMIC    2
#define ELF_SEGMENT_TYPE_INTERP     3
#define ELF_SEGMENT_TYPE_NOTE       4

#define ELF_SEGMENT_FLAG_EXEC       1
#define ELF_SEGMENT_FLAG_WRITE      2
#define ELF_SEGMENT_FLAG_READ       4

#define ELF64_SYM_BIND(i) ((i) >> 4)
#define ELF64_SYM_TYPE(i) ((i) & 0xF)

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2

#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3
#define STT_FILE        4

#define SHN_UNDEF       0

char* elf_get_string_at(char* image, uint32_t string_index);

struct elf_section_header_entry* elf_get_section_entry(char* image, uint32_t section_index);

struct elf_program_header_entry* elf_get_program_entry(char* image, uint32_t program_index);

#endif