#include "elf64.h"
#include "string.h"

int elf_check_signature(elf_header_t* elf_header)
{
    return elf_header->header[0] == 0x7F && !memcmp(&elf_header->header[1], "ELF", 3);
}

elf_section_header_entry_t* elf_get_section_entry(char* image, uint32_t section_index)
{
    elf_header_t* elf_header = (elf_header_t*)image;
    return (elf_section_header_entry_t*)
        (image + elf_header->section_header_table_pos + section_index * elf_header->section_header_entry_size);
}

elf_program_header_entry_t* elf_get_program_entry(char* image, uint32_t program_index)
{
    elf_header_t* elf_header = (elf_header_t*)image;
    return (elf_program_header_entry_t*)
        (image + elf_header->prog_header_table_pos + program_index * elf_header->prog_header_entry_size);

}

char* elf_get_string_at(char* image, uint32_t string_index)
{
    elf_header_t* elf_header = (elf_header_t*)image;
    elf_section_header_entry_t* string_section = elf_get_section_entry(image, elf_header->section_names_index);
    return image + string_section->offset + string_index;
}

void elf_read_sections(char* image, elf_sections_ptr_t* sections_struct)
{
    memset(sections_struct, 0, sizeof(elf_sections_ptr_t));
    elf_header_t* elf_header = (elf_header_t*)image;

    for (uint32_t i = 0; i < elf_header->section_header_entries_num; i ++) {
        elf_section_header_entry_t* sehentry = elf_get_section_entry(image, i);

        char* section_name = elf_get_string_at(image, sehentry->name_offset);
        if (strcmp(section_name, ".dynamic") == 0) {
            sections_struct->dynamic_ptr = sehentry;
        } else if (strcmp(section_name, ".dynsym") == 0) {
            sections_struct->dynsym_ptr = sehentry;
        } else if (strcmp(section_name, ".dynstr") == 0) {
            sections_struct->dynstr_ptr = sehentry;
        } else if (strcmp(section_name, ".rela.plt") == 0) {
            sections_struct->rela_plt_ptr = sehentry;
        } else if (strcmp(section_name, ".comment") == 0) {
            sections_struct->comment_ptr = sehentry;
        } else if (strcmp(section_name, ".tbss") == 0 && (sehentry->flags & ELF_SECTION_FLAG_TLS)) {
            sections_struct->tbss_ptr = sehentry;
        } else if (strcmp(section_name, ".tdata") == 0 && (sehentry->flags & ELF_SECTION_FLAG_TLS)) {
            sections_struct->tdata_ptr = sehentry;
        } 

        /*printf("SEC: name %s foffset %i size %i type %i flags %i\n", 
                elf_get_string_at(image, sehentry->name_offset),
                sehentry->offset,
                sehentry->size,
                sehentry->type,
                sehentry->flags);*/
    }
}