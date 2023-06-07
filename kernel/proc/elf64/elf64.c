#include "elf64.h"
#include "string.h"

int elf_check_signature(elf_header_t* elf_header)
{
    return elf_header->header[0] == 0x7F && !memcmp(&elf_header->header[1], "ELF", 3);
}

elf_section_header_entry_t* elf_get_section_entry(char* image, uint32_t section_index)
{
    elf_header_t* elf_header = (elf_header_t*)image;
    return (elf_section_header_entry_t*)(image + elf_header->section_header_table_pos + section_index * elf_header->section_header_entry_size);
}

char* elf_get_string_at(char* image, uint32_t string_index)
{
    elf_header_t* elf_header = (elf_header_t*)image;
    elf_section_header_entry_t* string_section = elf_get_section_entry(image, elf_header->section_names_index);
    return image + string_section->offset + string_index;
}