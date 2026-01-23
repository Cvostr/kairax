#include "aml.h"
#include "acpi_tables.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "kairax/stdio.h"
#include "kairax/errors.h"
#include "kairax/string.h"
#include "kairax/kstdlib.h"
#include "io.h"
#include "dev/bus/pci/pci.h"
#include "memory/paging.h"
#include "memory/kernel_vmm.h"

void aml_acquire_node(struct aml_node *node)
{
    atomic_inc(&node->refs);
}

void aml_free_node(struct aml_node *node)
{
    if (node == NULL)
        return;

    if (atomic_dec_and_test(&node->refs))
    {
        //printk("Freeing AML Node %p\n", node);
        switch (node->type)
        {
        case STRING:
            if (node->string.str)
                kfree(node->string.str);
            break;
        case BUFFER:
            if (node->buffer.buffer)
                kfree(node->buffer.buffer);
            break;
        case PACKAGE:
            if (node->package.elements != NULL)
            {
                // TODO: free all elements
                kfree(node->package.elements);
            }
            break;
        case METHOD:
            break;
        default:
            break;
        }

        kfree(node);
    }
}

int aml_to_uint64(struct aml_node* node, uint64_t *out)
{
    int res = 0;
    switch (node->type)
    {
    case PACKAGE:
        printk("ACPI: Should PACKAGE be convertible to Integer?\n");
        return -EINVAL;
    case STRING:
        *out = atol(node->string.str);
        return 0;
    default:
        res = aml_node_as_integer(node, out);
        if (res != 0)
        {
            printk("ACPI: Error getting node type %i as integer. rc = %i\n", node->type, res);
        }
        
        return res;
    }
}

int aml_store_to_node(struct aml_node *value, struct aml_node *dest)
{
    int res;
    switch (dest->type)
    {
        case INTEGER:
            uint64_t converted;
            res = aml_to_uint64(value, &converted);
            if (res != 0)
            {
                printk("ACPI: Failed to convert value to Integer: %i\n", res);
                return res;
            }

            dest->int_value = converted;
            break;
        default:
            printk("ACPI: aml_store_to_node: Unsupported for type %i\n", dest->type);
            return -ENOTSUP;
    }

    return 0;
}

int acpi_evaluate_value(struct aml_node* scope, struct aml_node* node, struct aml_node **val)
{
    int res = 0;
    switch (node->type)
    {
        case METHOD:
            // Конструируем фиктивный контекст
            // Поскольку аргументов нет, то просто заполним только scope
            struct aml_ctx fake_ctx;
            memset(&fake_ctx, 0, sizeof(struct aml_ctx));
            fake_ctx.scope = scope;

            res = aml_execute_method(&fake_ctx, node, FALSE, val);
            break;
        default:
            // делаем копию
            struct aml_node* resnode = aml_make_node(NONE);
            memcpy(resnode, node, sizeof(struct aml_node));
            *val = resnode; 
            break;
    }

    return res;
}

// Page 1027
int aml_map_access_type_to_size(uint8_t flags)
{
    uint8_t access_type = flags & 0b1111;
    // bit 0-3: AccessType 
    // 0 AnyAcc
    // 1 ByteAcc
    // 2 WordAcc
    // 3 DWordAcc
    // 4 QWordAcc
    switch (access_type)
    {
    case 1: return 1;
    case 2: return 2;
    case 3: return 4;
    case 4: return 8;
    }

    return 0;
}

int aml_node_as_integer(struct aml_node *node, uint64_t *result)
{
    int rc;

    if (node == NULL)
        return -EINVAL;

    if (node->type == INTEGER)
    {
        *result = node->int_value;
        return 0;
    }

    if (node->type == FIELD)
    {
        // Проверим, что размер поля не больше 64 бит. Если больше, то это не integer
        size_t field_sz = node->field.len;
        if (field_sz > 64)
        {
            return -EINVAL;
        }

        // Обнулим вначале, вдруг вызвавший не обнулил
        *result = 0;

        rc = aml_read_from_field(node, result, field_sz / 8);

        if (rc != 0)
        {
            printk("ACPI: Read from field failed (%i)!\n", rc);
            return rc;
        }

        return 0;
    }

    printk("ACPI: Can't cast node type %i to Integer\n", node->type);
    return -EINVAL;
}

// len в байтах
int aml_read_from_field(struct aml_node *field, uint8_t *data, size_t len)
{
    int rc = 0;
    int access = aml_map_access_type_to_size(field->field.flags);
    switch (field->field.type)
    {
        case DEFAULT:
            // Чтение из OpRegion
            rc = aml_read_from_op_region(field->field.mem.opregion, field->field.offset / 8, len, access, data);
            break;
        case INDEX:
            // Запись смещения в поле Index
            struct aml_node *index_field = field->field.mem.index.index;
            struct aml_node *data_field = field->field.mem.index.data;
            uint64_t bytes_offset = field->field.offset / 8;
            rc = aml_write_to_field(index_field, &bytes_offset, index_field->field.len / 8);
            if (rc != 0)
            {
                printk("ACPI: Error during writing offset to index field: %i\n", rc);
                return rc;
            }

            rc = aml_read_from_field(data_field, data, len);
            break;
        default:
            printk("ACPI: Write to field type %i not implemented!\n", field->field.type);
            return -EINVAL;
    }

    return rc;
}

// len в байтах
int aml_write_to_field(struct aml_node *field, const uint8_t *data, size_t len)
{
    int rc;
    int access = aml_map_access_type_to_size(field->field.flags);
    switch (field->field.type)
    {
        case DEFAULT:
            // Запись в OpRegion
            rc = aml_write_to_op_region(field->field.mem.opregion, field->field.offset / 8, len, access, data);
            break;
        case INDEX:
            // Запись смещения в поле Index
            struct aml_node *index_field = field->field.mem.index.index;
            struct aml_node *data_field = field->field.mem.index.data;
            uint64_t bytes_offset = field->field.offset / 8;
            rc = aml_write_to_field(index_field, &bytes_offset, index_field->field.len / 8);
            if (rc != 0)
            {
                printk("ACPI: Error during writing offset to index field: %i\n", rc);
                return rc;
            }

            rc = aml_write_to_field(data_field, data, len);
            break;
        default:
            printk("ACPI: Write to field type %i not implemented!\n", field->field.type);
            return -EINVAL;
    }

    return rc;
}

int aml_execute_method(struct aml_ctx *ctx, struct aml_node *method, int read_args, struct aml_node **returned_node)
{
    int rc;
    uint8_t opcode;
    struct aml_node *arg_node;

    printk("ACPI: Executing method\n");

    // Конструируем контекст
    struct aml_ctx method_ctx;
    memset(&method_ctx, 0, sizeof(struct aml_ctx));
    method_ctx.aml_data = method->method.code;
    method_ctx.aml_len = method->method.code_size;
    method_ctx.current_pos = 0;
    method_ctx.scope = ctx->scope;

    if (read_args == TRUE)
    {
        for (size_t i = 0; i < method->method.args; i ++)
        {
            // Считаем очередной аргумент
            rc = aml_parse_next_node(ctx, &arg_node);
            if (rc != 0)
            {
                printk("ACPI: Error parsing next argument node (%i)\n", rc);
                return rc;
            }

            // Сохраним аргумент в контекст
            method_ctx.args[i] = arg_node;
        }
    }

    // TODO: Mutex, если synchronized

    // Парсим метод
    struct aml_node *node;
    while (aml_ctx_get_remain_size(&method_ctx) > 0)
    {
        rc = aml_parse_next_node(&method_ctx, &node);
        if (rc == (0xFF00 | AML_OP_RETURN))
        {
            // Выход из метода
            *returned_node = node;
            rc = 0;
            printk("ACPI: method return catch\n");
            break;
        }
        else if (rc != 0)
        {
            printk("ACPI: Error parsing next node during method execution (%i)\n", rc);
            return rc;
        }
    }

    return rc;
}

/// -- РАБОТА С OP REGION

int aml_read_from_op_region(struct aml_node *region, size_t offset, size_t len, uint8_t access_sz, uint8_t *out)
{
    //printk("aml_read_from_op_region acc_sz %i len %i\n", access_sz, len);
    
    // Защита от деления на 0
    if (access_sz == 0)
        access_sz = 1;

    int rc = 0;
    size_t blocks = len / access_sz;
    size_t remain_bytes = len % access_sz;
    uint64_t tmp;
    size_t written_bytes = 0;

    // Сначала чтение по блокам с размером access_sz
    for (size_t i = 0; i < blocks; i ++)
    {
        rc = aml_read_integer_from_op_region(region, offset + written_bytes, access_sz, &tmp);
        if (rc != 0)
            return rc;
        
        memcpy(out + written_bytes, &tmp, access_sz);
        written_bytes += access_sz;
    }

    // Чтение оставшегося
    if (remain_bytes > 0)
    {
        rc = aml_read_integer_from_op_region(region, offset + written_bytes, access_sz, &tmp);
        if (rc != 0)
            return rc;

        memcpy(out + written_bytes, &tmp, remain_bytes);
    }

    return rc;
}

int aml_write_to_op_region(struct aml_node *region, size_t offset, size_t len, uint8_t access_sz, uint8_t *in)
{
    //printk("aml_write_to_op_region acc_sz %i len %i\n", access_sz, len);
    int rc = 0;

    // Защита от деления на 0
    if (access_sz == 0)
        access_sz = 1;

    size_t blocks = len / access_sz;
    size_t remain_bytes = len % access_sz;
    uint64_t tmp;
    size_t written_bytes = 0;

    // Сначала запись по блокам с размером access_sz
    for (size_t i = 0; i < blocks; i ++)
    {
        memcpy(&tmp, in + written_bytes, access_sz);

        rc = aml_write_integer_to_op_region(region, offset + written_bytes, access_sz, tmp);
        if (rc != 0)
            return rc;
        
        written_bytes += access_sz;
    }

    // Запись оставшегося
    if (remain_bytes > 0)
    {
        memcpy(&tmp, in + written_bytes, remain_bytes);

        rc = aml_write_integer_to_op_region(region, offset + written_bytes, access_sz, tmp);
        if (rc != 0)
            return rc;
    }

    return rc;
}

int aml_get_pci_config_space(struct aml_node *region, uint8_t *bus, uint8_t *dev, uint8_t *func)
{
    int rc;
    struct ns_node *bbn;
    struct ns_node *adr;
    
    bbn = acpi_ns_get_node1(acpi_get_root_ns(), region->op_region.scope, "_BBN");
    if (bbn == NULL)
    {
        printk("ACPI: aml_get_pci_config_space: _BBN node not found\n");
        return -ENOENT;
    }

    adr = acpi_ns_get_node1(acpi_get_root_ns(), region->op_region.scope, "_ADR");
    if (adr == NULL)
    {
        printk("ACPI: aml_get_pci_config_space: _ADR node not found\n");
        return -ENOENT;
    }

/*
    struct aml_node *bbn_node = bbn->object;
    struct aml_node *adr_node = adr->object;
    printk("BBN type %i ADR type %i\n", bbn_node->type, adr_node->type);
*/
    struct aml_node *bbn_val_node, *adr_val_node;
    uint64_t bbn_val, adr_val;
    
    rc = acpi_evaluate_value(region->op_region.scope, bbn->object, &bbn_val_node);
    if (rc != 0)
    {
        printk("ACPI: Error evaluating _BBN node (%i)\n", rc);
        return rc;
    }

    rc = acpi_evaluate_value(region->op_region.scope, adr->object, &adr_val_node);
    if (rc != 0)
    {
        printk("ACPI: Error evaluating _ADR node (%i)\n", rc);
        return rc;
    }

    rc = aml_to_uint64(bbn_val_node, &bbn_val);
    if (rc != 0)
    {
        printk("ACPI: Error casting _BBN node to Integer (%i)\n", rc);
        return rc;
    }

    rc = aml_to_uint64(adr_val_node, &adr_val);
    if (rc != 0)
    {
        printk("ACPI: Error casting _ADR node to Integer (%i)\n", rc);
        return rc;
    }

    //printk("BBN: %i ADR: %i\n", bbn_val, adr_val);

    *bus = bbn_val;
    *dev = adr_val >> 16;
    *func = adr_val & 0xFF;

    return 0;
}

int aml_read_integer_from_op_region(struct aml_node *region, size_t offset, uint8_t access_sz, uint64_t *out)
{
    int rc;
    uintptr_t addr_val = region->op_region.offset + offset;
    //printk("ACPI: aml_read_integer_from_op_region start accsz %i addr %p\n", access_sz, addr_val);
    switch (region->op_region.space)
    {
        case ACPI_GAS_MEMORY:
            //printk("ACPI: aml_read_integer_from_op_region: System Memory\n");
            void *vaddr = arch_phys_to_virtual_addr(addr_val);
            switch (access_sz)
            {
                case 1:
                    *out = *(uint8_t*)(vaddr); 
                    break;
                case 2:
                    *out = *(uint16_t*)(vaddr); 
                    break;
                case 4:
                    *out = *(uint32_t*)(vaddr); 
                    break;
                case 8:
                    *out = *(uint64_t*)(vaddr); 
                    break;
                default:
                    return -EINVAL;
            }
            break;
        case ACPI_GAS_IO:
            switch (access_sz)
            {
                case 1:
                    *out = inb(addr_val);
                    break;
                case 2:
                    *out = inw(addr_val); 
                    break;
                case 4:
                    *out = inl(addr_val); 
                    break;
                default:
                    return -EINVAL;
            }
            break;
        case ACPI_GAS_PCI_CONF:
            uint8_t bus, dev, func;
            rc = aml_get_pci_config_space(region, &bus, &dev, &func);
            if (rc != 0)
            {
                printk("ACPI: Error getting PCI Device Address (%i)\n", rc);
                return rc;
            }

            switch (access_sz)
            {
                case 1:
                    *out = i_pci_config_read8(bus, dev, func, addr_val);
                    break;
                case 2:
                    *out = i_pci_config_read16(bus, dev, func, addr_val);
                    break;
                case 4:
                    *out = i_pci_config_read32(bus, dev, func, addr_val);
                    break;
                default:
                    return -EINVAL;
            }
            
            break;
        default:
            return -EINVAL;
    }

    return 0;
}


int aml_write_integer_to_op_region(struct aml_node *region, size_t offset, uint8_t access_sz, uint64_t in)
{
    uintptr_t addr_val = region->op_region.offset + offset;
    switch (region->op_region.space)
    {
        case ACPI_GAS_MEMORY:
            void *vaddr = arch_phys_to_virtual_addr(addr_val);
            switch (access_sz)
            {
                case 1:
                    *(uint8_t*)(vaddr) = in; 
                    break;
                case 2:
                    *(uint16_t*)(vaddr) = in; 
                    break;
                case 4:
                    *(uint32_t*)(vaddr) = in; 
                    break;
                case 8:
                    *(uint64_t*)(vaddr) = in; 
                    break;
                default:
                    return -EINVAL;
            }
            break;
        case ACPI_GAS_IO:
            switch (access_sz)
            {
                case 1:
                    outb(addr_val, in);
                    break;
                case 2:
                    outw(addr_val, in); 
                    break;
                case 4:
                    outl(addr_val, in); 
                    break;
                default:
                    return -EINVAL;
            }
            break;
        case ACPI_GAS_PCI_CONF:
            printk("ACPI: PCI Conf space is not implemented!\n");
            return -ENOSYS;
        default:
            return -EINVAL;
    }

    return 0;
}