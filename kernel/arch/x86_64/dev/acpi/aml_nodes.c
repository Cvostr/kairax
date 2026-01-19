#include "aml.h"
#include "acpi_tables.h"
#include "mem/paging.h"
#include "kairax/stdio.h"
#include "kairax/errors.h"
#include "kairax/string.h"
#include "kairax/kstdlib.h"
#include "io.h"

void aml_free_node(struct aml_node *node)
{
    if (node == NULL)
        return;
    
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
    case METHOD:
        kfree(node->method.code);
        break;
    default:
        break;
    }

    kfree(node);
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

int aml_execute_method(struct aml_ctx *ctx, struct aml_node *method)
{
    int rc;
    struct aml_node *arg_node;

    printk("ACPI: Executing method\n");

    for (size_t i = 0; i < method->method.args; i ++)
    {
        rc = aml_parse_next_node(ctx, &arg_node);
        if (rc != 0)
        {
            printk("ACPI: Error parsing next argument node (%i)\n", rc);
            return rc;
        }
    }

    struct aml_ctx method_ctx;
    method_ctx.aml_data = method->method.code;
    method_ctx.aml_len = method->method.code_size;
    method_ctx.current_pos = 0;
    method_ctx.scope = ctx->scope;

    // Парсим метод
    struct aml_node *node;
    while (aml_ctx_get_remain_size(&method_ctx) > 0)
    {
        rc = aml_parse_next_node(&method_ctx, &node);
        if (rc != 0)
        {
            printk("ACPI: Error parsing next node during method execution (%i)\n", rc);
            return rc;
        }
    }

    return -1;
}

/// -- РАБОТА С OP REGION

int aml_read_from_op_region(struct aml_node *region, size_t offset, size_t len, uint8_t access_sz, uint8_t *out)
{
    //printk("aml_read_from_op_region acc_sz %i len %i\n", access_sz, len);
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

    // Запись оставшегося
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

int aml_read_integer_from_op_region(struct aml_node *region, size_t offset, uint8_t access_sz, uint64_t *out)
{
    uintptr_t addr_val = region->op_region.offset + offset;
    switch (region->op_region.space)
    {
        case ACPI_GAS_MEMORY:
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
            printk("ACPI: PCI Conf space is not implemented!\n");
            return -ENOSYS;
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