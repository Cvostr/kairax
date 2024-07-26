#ifndef __ELF_H
#define __ELF_H

#include <sys/types.h>
#include "elf.h"

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

#define ELF_SEGMENT_TYPE_IGNORED    0
#define ELF_SEGMENT_TYPE_LOAD       1
#define ELF_SEGMENT_TYPE_DYNAMIC    2
#define ELF_SEGMENT_TYPE_INTERP     3
#define ELF_SEGMENT_TYPE_NOTE       4

#define ELF_SEGMENT_FLAG_EXEC       1
#define ELF_SEGMENT_FLAG_WRITE      2
#define ELF_SEGMENT_FLAG_READ       4

char* elf_get_string_at(char* image, uint32_t string_index);

struct elf_section_header_entry* elf_get_section_entry(char* image, uint32_t section_index);

struct elf_program_header_entry* elf_get_program_entry(char* image, uint32_t program_index);

#endif