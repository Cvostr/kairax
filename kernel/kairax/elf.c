#include "./elf.h"
#include "string.h"

int elf_check_signature(struct elf_header* elf_header)
{
    return elf_header->header[0] == 0x7F && !memcmp(&elf_header->header[1], "ELF", 3);
}

struct elf_section_header_entry* elf_get_section_entry(const char* image, uint32_t section_index)
{
    struct elf_header* elf_header = (struct elf_header*)image;
    return (struct elf_section_header_entry*)
        (image + elf_header->section_header_table_pos + section_index * elf_header->section_header_entry_size);
}

struct elf_program_header_entry* elf_get_program_entry(char* image, uint32_t program_index)
{
    struct elf_header* elf_header = (struct elf_header*)image;
    return (struct elf_program_header_entry*)
        (image + elf_header->prog_header_table_pos + program_index * elf_header->prog_header_entry_size);
}

char* elf_get_string_at(const char* image, uint32_t string_index)
{
    struct elf_header* elf_header = (struct elf_header*) image;
    struct elf_section_header_entry* string_section = elf_get_section_entry(image, elf_header->section_names_index);
    return image + string_section->offset + string_index;
}
