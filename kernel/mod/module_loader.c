#include "module_loader.h"
#include "kairax/elf.h"
#include "errors.h"
#include "module_stor.h"
#include "string.h"
#include "function_table.h"
#include "kairax/stdio.h"
#include "kstdlib.h"

struct kxmodule_header
{
    char mod_name[20];
    int (*mod_init_routine)(void);
    void (*mod_destroy_routine)(void);
};

int module_load(const char* image, size_t size)
{
    int rc = 0;
    struct elf_header* elf_header = (struct elf_header*) image;

    if (!elf_check_signature(elf_header)) {
        return -ERROR_BAD_EXEC_FORMAT;
    }

    struct kxmodule_header* module_header_ptr = NULL;
    struct elf_section_header_entry* symtab_section = NULL;
    struct elf_section_header_entry* strtab_section = NULL;

    struct elf_section_header_entry* rela_sections[6];
    int rela_sections_num = 0;

    // Чтение секций, поиск заголовка
    for (uint32_t i = 0; i < elf_header->section_header_entries_num; i ++) {
        // Получить описание секции
        struct elf_section_header_entry* sehentry = elf_get_section_entry(image, i);
        // Получить название секции
        char* section_name = elf_get_string_at(image, sehentry->name_offset);

        if (strcmp(section_name, ".kxmod_header") == 0) {
            // Заголочная секция
            module_header_ptr = (struct kxmodule_header*) sehentry->offset;
        } else if (sehentry->type == SHT_SYMTAB) {
            // Секция перемещений в коде
            symtab_section = sehentry;
        } else if (strcmp(section_name, ".strtab") == 0) {
            // Секция перемещений в коде
            strtab_section = sehentry;
        } else if(sehentry->type == SHT_RELA) {
            // Это одна из секций перемещений
            // Добавляем её в массив
            rela_sections[rela_sections_num++] = sehentry;
        }
    }

    if (module_header_ptr == NULL) {
        // В модуле отсутствует секция kxmod_header. Без нее делать что-либо дальше нет смысла
        return -ERROR_BAD_EXEC_FORMAT;
    }

    char* module_name = ((struct kxmodule_header*) (image + (uint64_t) module_header_ptr))->mod_name;

    struct module* mod = mstor_new_module(size, module_name);
    mod->state = MODULE_STATE_LOADING;
    if (mod == NULL) {
        return -1;
    }

    rc = mstor_register_module(mod);
    if (rc != 0) {
        free_module(mod);
        goto exit;
    }

    // Копируем данные в память рядом с кодом ядра
    memcpy((void*) mod->offset, image, size);

    // Применение перемещений
    struct elf_symbol* sym_data = (struct elf_symbol*) (image + symtab_section->offset);

    for (int rel_section_i = 0; rel_section_i < rela_sections_num; rel_section_i ++) {
        struct elf_section_header_entry* rela_section = rela_sections[rel_section_i];
        // К данным в этой секции будут применяться перемещения
        struct elf_section_header_entry* relocating_section = elf_get_section_entry(image, rela_section->info);
        struct elf_rela* relocs_data = (struct elf_rela*) (image + rela_section->offset);

        for (size_t i = 0; i < rela_section->size / sizeof(struct elf_rela); i ++) {
            // Указатель на перемещение
            struct elf_rela* rela = relocs_data + i;
            int relocation_type = ELF64_R_TYPE(rela->info);
            int relocation_sym_index = ELF64_R_SYM(rela->info);
            
            //printk("type %i, off %i, num %i\n", relocation_type, rela->offset, relocation_sym_index);

            struct elf_symbol* sym = sym_data + relocation_sym_index;
            const char* sym_name = image + strtab_section->offset + sym->name;
            //printk("sym name %s indx: %i\n", sym_name, sym->shndx);

            if (sym->shndx == 0) {
                // символ не определен в модуле - значит он может быть в ядре
                void* func_ptr = kfunctions_get_by_name(sym_name);
                if (func_ptr == NULL) {
                    // todo : искать в других модулях
                    printk("error locating symbol %s\n", sym_name);

                    return -1;
                }

                //todo : архитектурно - зависимый код. разделить!
                if (relocation_type == R_X86_64_PLT32) {
                    // Смещение, по которому нужно применить перемещение
                    int* value = (int*) (mod->offset + relocating_section->offset + rela->offset);
                    int jmpoffset = (int) ((uint64_t) func_ptr - (uint64_t) value);
                    *value = jmpoffset;
                }
            } else {
                // символ определен в модуле.
                // Получаем секцию, в которой расположены данные символа
                struct elf_section_header_entry* sym_section = elf_get_section_entry(image, sym->shndx);
                //todo : архитектурно - зависимый код. разделить!
                switch (relocation_type) {
                    case R_X86_64_PC32:
                    case R_X86_64_PLT32:
                        // Куда применить перемещение
                        int* value = (int*) (mod->offset + relocating_section->offset + rela->offset);
                        int jmpoffset = (int) ((uint64_t) (mod->offset + sym_section->offset + sym->value + rela->addend) - (uint64_t) value);
                        *value = jmpoffset;
                        break;
                    case R_X86_64_64:
                        uint64_t* value64 = (uint64_t*) (mod->offset + relocating_section->offset + rela->offset);
                        *value64 = mod->offset + sym_section->offset + sym->value + rela->addend;
                        break;
                }
            }
        }
    }

    module_header_ptr = (struct kxmodule_header*) (mod->offset + (uint64_t) module_header_ptr);
    mod->mod_init_routine = module_header_ptr->mod_init_routine;
    mod->mod_destroy_routine = module_header_ptr->mod_destroy_routine;

    if (mod->mod_init_routine)
        rc = mod->mod_init_routine();

    mod->state = MODULE_STATE_READY; 
    
exit:
    return rc;
}