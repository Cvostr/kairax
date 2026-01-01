#include "multiboot.h"
#include "string.h"
#include "kairax/stdio.h"
#include "memory/mem_layout.h"
#include "mod/module_loader.h"
#include "mem/pmm.h"
#include "kairax/kstdlib.h"

#define MBOOT_REPLY 0x36D76289

#define MBOOT2_COMMANDLINE  1
#define MBOOT2_BOOTLOADER   2
#define MBOOT2_MODULE       3
#define MBOOT2_MMAP         6
#define MBOOT2_VBE_INFO     7
#define MBOOT2_FB_INFO      8
#define MBOOT2_IMAGE_BASE   21
#define MBOOT2_RSDP1        14
#define MBOOT2_RSDP2        15

static kernel_boot_info_t kernel_boot_info;

int parse_mb2_tags(taglist_t *tags, size_t* total_size)
{
    *total_size = tags->total_size;
    memset(&kernel_boot_info, 0, sizeof(kernel_boot_info));
    tag_t *tag = tags->tags;
    mmap_t *mmap;
    int index = 0;
    while(tag->type)
    {
        switch(tag->type)
        {
        case MBOOT2_BOOTLOADER:
            kernel_boot_info.bootloader_string = (char*) tag->data;
            break;
        case MBOOT2_COMMANDLINE:
            kernel_boot_info.command_line = (char*) tag->data;
            break;
        case MBOOT2_MODULE:
            module_t* mod = tag->data;
            size_t modsize = mod->mod_end - mod->mod_start;
            // Даем временную защиту области памяти, в которую загружен модуль
            pmm_set_mem_region(mod->mod_start, modsize, TRUE);
            break;
        case MBOOT2_MMAP:
            mmap = kernel_boot_info.mmap = (mmap_t*)tag->data;
            kernel_boot_info.mmap_len = (tag->size - 8) / mmap->entry_size;
            kernel_boot_info.mmap_size = (tag->size - 8);
            break;
        case MBOOT2_IMAGE_BASE:
            kernel_boot_info.load_base_addr = *(uint32_t*)tag->data;
            break;
        case MBOOT2_FB_INFO:
            memcpy(&kernel_boot_info.fb_info, (framebuffer_info_t*)tag->data, sizeof(framebuffer_info_t));
            break;
        case MBOOT2_RSDP1:
            kernel_boot_info.rsdp_version = 1;
            memcpy(kernel_boot_info.rsdp_data, tag->data, tag->size);
            break;
        case MBOOT2_RSDP2:
            kernel_boot_info.rsdp_version = 2;
            memcpy(kernel_boot_info.rsdp_data, tag->data, tag->size);
            break;
        }
        tag = (tag_t*) ((uint8_t*) tag + ((tag->size + 7) & ~7));
    }
    return 0;
}

int mb2_load_modules(taglist_t *tags)
{
    tag_t *tag = tags->tags;
    int index = 0;
    while(tag->type)
    {
        switch(tag->type)
        {
        case MBOOT2_MODULE:
            module_t* mod = tag->data;
            size_t modsize = mod->mod_end - mod->mod_start;
            size_t mod_pages = modsize / PAGE_SIZE; //align(modsize, PAGE_SIZE) / PAGE_SIZE;

            // Прогрузка модуля в ядро
            printk("Loading module %s, size %i\n", mod->name, modsize);
            int rc = module_load(P2V(mod->mod_start), modsize);
            if (rc != 0)
            {
                printk("Error loading module: %i\n", -rc);
            }

            // Зануляем память
            memset(P2V(mod->mod_start), 0, modsize);
            // Снимаем временную защиту
            pmm_free_pages(mod->mod_start, mod_pages);
            break;
        }
        tag = (tag_t*) ((uint8_t*) tag + ((tag->size + 7) & ~7));
    }
    return 0;
}

kernel_boot_info_t* get_kernel_boot_info()
{
    return &kernel_boot_info;
}

int multiboot_get_memory_area(size_t entry_index, uintptr_t *start, uintptr_t *length, uint32_t *type)
{
    if(entry_index >= kernel_boot_info.mmap_len) return 1;

    mmap_t *mmap = kernel_boot_info.mmap;
    mmap_entry_t *entry = &mmap->entries[entry_index];

    *start = entry->base;
    *length = entry->len;
    *type = entry->type;
    return 0;
}