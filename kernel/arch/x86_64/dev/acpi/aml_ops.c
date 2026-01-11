#include "aml.h"
#include "kairax/stdio.h"
#include "kairax/string.h"
#include "mem/kheap.h"

int aml_op_alias(struct aml_ctx *ctx)
{
    printk("ALIAS not SUPPORTED!!!\n");
    return -ENOTSUP;
}

int aml_op_scope(struct aml_ctx *ctx)
{
    int rc;
    size_t len;
    uint8_t *nested = NULL;

    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &nested);

    struct aml_ctx parse_ctx;
    parse_ctx.aml_data = nested;
    parse_ctx.aml_len = len;
    parse_ctx.current_pos = 0;

    struct aml_name_string *ns_name = aml_read_name_string(&parse_ctx);
    printk("SCOPE pkg_len %i, name (root=%i, base=%i, segments=%i)\n", len, ns_name->from_root, ns_name->base, ns_name->segments_num);

    // Попробуем получить объект по считанному имени
    struct ns_node *scope_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, ns_name);
    if (scope_node == NULL)
    {
        printk("ACPI: ScopeOp: Unable to find scope node (%s) in namespace\n", aml_debug_namestring(ns_name));
        return -ENOENT;
    }

    // TODO: Надо бы проверить тип объекта

    // Устанавливаем новый scope для нового контекста
    parse_ctx.scope = scope_node;

    // Парсим контекст
    struct aml_node *node;
    while (parse_ctx.current_pos < parse_ctx.aml_len)
    {
        rc = aml_parse_next_node(&parse_ctx, &node);
        if (rc != 0)
        {
            printk("ACPI: ScopeOp: Error parsing next node (%i)\n", rc);
            return rc;
        }
    }

    return 0;
}

int aml_op_region_op(struct aml_ctx *ctx)
{
    int rc;
    struct aml_node *region_offset_node = NULL, *region_len_node = NULL;
    struct aml_name_string *region_name = NULL;

    // Считаем название региона
    region_name = aml_read_name_string(ctx);
    // Вид памяти, в которой располагается регион
    uint8_t region_space = aml_ctx_get_byte(ctx);
        
    //printk("OP REGION OP name %s reg space 0x%x\n", region_name->segments->seg_s, region_space);

    rc = aml_parse_next_node(ctx, &region_offset_node);
    if (rc != 0)
    {
        printk("ACPI: OpRegionOp: Error reading RegionOffset node (%i)\n", rc);
        goto exit;
    }

    if (region_offset_node->type != INTEGER)
    {
        printk("ACPI: BufferOp: BufferSize node has incorrect type (%i)\n", region_offset_node->type);
        rc = -EINVAL;
        goto exit;
    }
    
    rc = aml_parse_next_node(ctx, &region_len_node);
    if (rc != 0)
    {
        printk("ACPI: OpRegionOp: Error reading RegionLen node (%i)\n", rc);
        goto exit;
    }

    if (region_len_node->type != INTEGER)
    {
        printk("ACPI: BufferOp: BufferSize node has incorrect type (%i)\n", region_len_node->type);
        rc = -EINVAL;
        goto exit;
    }

    struct aml_node *region_node = aml_make_node(OP_REGION);
    region_node->op_region.space = region_space;
    region_node->op_region.offset = region_offset_node->int_value;
    region_node->op_region.len = region_len_node->int_value;

    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, region_name, region_node);
    if (rc != 0)
    {
        printk("ACPI: OpRegionOp: Error adding node to namespace (%i)\n", rc);
        goto exit;
    }

    //printk("\toffset type %i value 0x%x\n", region_offset_node->type, region_offset_node->int_value);
    //printk("\tlen type %i len 0x%x\n", region_len_node->type, region_len_node->int_value);

exit:
    KFREE_SAFE(region_len_node);
    KFREE_SAFE(region_offset_node);
    KFREE_SAFE(region_name);
    return 0;
}

// 19.6.47 Field (Declare Field Objects)
int aml_parse_field_list(struct aml_ctx *ctx)
{
    uint8_t cur; 
    uint64_t offset_bits;
    
    while (ctx->current_pos < ctx->aml_len)
    {
        cur = aml_ctx_peek_byte(ctx);
        switch (cur)
        {
            case 0:
                aml_ctx_get_byte(ctx);
                uint32_t offset = aml_read_pkg_len(ctx);
                printk("\tReserved field. Offset %x (%i)\n", offset, offset);
                offset_bits += offset;
                break;
            case 1:
                aml_ctx_get_byte(ctx);
                uint8_t access_type = aml_ctx_get_byte(ctx);
                uint8_t access_attrib = aml_ctx_get_byte(ctx);
                printk("\taccess field. access_type %i access_attrib %i\n");
                break;
            case 2:
                printk("\tConnect field. Not implemented\n");
                return -ENOSYS;
                break;
            case 3:
                printk("\tExtended access field. Not implemented\n");
                return -ENOSYS;
                break;
            default:
                char seg[5];
                seg[4] = 0;
                aml_ctx_copy_bytes(ctx, seg, 4);
                uint32_t len = aml_read_pkg_len(ctx);
                printk("\tNamed field %s len %i\n", seg, len);
                offset += len;
                break;
        }
    }

    return 0;
}

// 20.2. AML Grammar Definition (998)
int aml_op_field(struct aml_ctx *ctx)
{
    int res = 0;
    struct aml_name_string *opregion_name = NULL;
    // Сделаем копию пакета
    size_t len;
    uint8_t *nested = aml_ctx_dup_from_pkg(ctx, &len);
    // Изза особенностей функций мы зависим от aml_ctx. 
    // Сделаем еще один фиктивный, ну а хули нет?
    struct aml_ctx field_ctx;
    field_ctx.aml_data = nested;
    field_ctx.aml_len = len;
    field_ctx.current_pos = 0;
    field_ctx.scope = ctx->scope;

    // Получим Operation Region по имени из namespace
    opregion_name = aml_read_name_string(&field_ctx);

    // Попробуем получить объект по считанному имени
    struct ns_node *opregion_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, opregion_name);
    if (opregion_node == NULL || opregion_node->object == NULL)
    {
        printk("ACPI: FieldOp: Unable to find node %s in namespace\n", opregion_name->segments->seg_s);
        res = -ENOENT;
        goto exit;
    }
    // Проверим тип
    if (opregion_node->object->type != OP_REGION)
    {
        printk("ACPI: BufferOp: BufferSize node has incorrect type (%i)\n", opregion_node->object->type);
        res = -EINVAL;
        goto exit;
    }

    uint8_t field_flags = aml_ctx_get_byte(&field_ctx);

    printk("FIELD OP pkg_len %i name '%s' flags %x\n", len, opregion_name->segments->seg_s, field_flags);

    // Выполним разбор структуры полей
    res = aml_parse_field_list(&field_ctx);

exit:
    KFREE_SAFE(nested);
    KFREE_SAFE(opregion_name);
    return res;
}

void aml_op_index_field(struct aml_ctx *ctx)
{   
    // Сделаем копию пакета
    size_t len;
    uint8_t *nested = aml_ctx_dup_from_pkg(ctx, &len);

    struct aml_ctx field_ctx;
    field_ctx.aml_data = nested;
    field_ctx.aml_len = len;
    field_ctx.current_pos = 0;

    struct aml_name_string *index_name = aml_read_name_string(&field_ctx);
    struct aml_name_string *data_name = aml_read_name_string(&field_ctx);
    uint8_t field_flags = aml_ctx_get_byte(&field_ctx);

    printk("INDEX FIELD OP pkg_len %i index_name '%s' data_name '%s' flags %x\n", 
        len, index_name->segments->seg_s, data_name->segments->seg_s, field_flags);

    aml_parse_field_list(&field_ctx);

    kfree(nested);
}

void aml_op_method(struct aml_ctx *ctx)
{
    size_t len;
    uint8_t *nested = aml_ctx_dup_from_pkg(ctx, &len);

    struct aml_ctx method_ctx;
    method_ctx.aml_data = nested;
    method_ctx.aml_len = len;
    method_ctx.current_pos = 0;
    method_ctx.scope = ctx->scope;

    struct aml_name_string *method_name = aml_read_name_string(&method_ctx);

    uint8_t method_flags = aml_ctx_get_byte(&method_ctx);
    uint8_t arg_count = method_flags & 0b111;
    uint8_t serialize = (method_flags > 3) & 1;
    uint8_t sync_level = (method_flags > 4) & 0xF;

    printk("METHOD OP pkg_len %i name '%s' args %i, serialized %i, sync level %i\n", 
        len, method_name->segments->seg_s, arg_count, serialize, sync_level);

    kfree(nested);
}

int aml_op_name(struct aml_ctx *ctx)
{
    // Считаем имя
    struct aml_name_string *name = aml_read_name_string(ctx);
    printk("OPNAME '%s'\n", name->segments->seg_s);

    // Распарсим ноду
    struct aml_node *value;
    int rc = aml_parse_next_node(ctx, &value);
    if (rc != 0)
    {
        printk("ACPI: NameOp: Error reading next node (%i)\n", rc);
        return rc;
    }

    if (value == NULL)
    {
        printk("ACPI: NameOp: next node is NULL\n");
        return -EINVAL;
    }

    // Добавляем считанную ноду с именем
    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, name, value);
    if (rc != 0)
    {
        printk("ACPI: NameOp: Error adding node to namespace (%i)\n", rc);
        return rc;
    }

    return 0;
}

struct aml_node *aml_op_package(struct aml_ctx *ctx)
{
    int rc;
    size_t len;
    uint8_t *pkg_buf = NULL;

    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &pkg_buf);

    struct aml_ctx pkg_ctx;
    pkg_ctx.aml_data = pkg_buf;
    pkg_ctx.aml_len = len;
    pkg_ctx.current_pos = 0;

    uint8_t num_elements = aml_ctx_get_byte(&pkg_ctx);

    //printk("OP PACKAGE len %i num_elements %i\n", len, num_elements);

    struct aml_node *node = aml_make_node(PACKAGE);
    node->package.num_elements = num_elements;
    node->package.elements = kmalloc(sizeof(struct aml_node *) * num_elements);

    struct aml_node *value;
    for (int i = 0; i < num_elements; i ++)
    {
        // TODO: строки и ссылки??
        rc = aml_parse_next_node(&pkg_ctx, &value);
        if (rc != 0)
        {
            printk("ACPI: Error (%i) handling package element\n", rc);
            return NULL;
        }

        node->package.elements[i] = value;
    }

    return node;
}

struct aml_node *aml_op_buffer(struct aml_ctx *ctx)
{
    int rc;
    size_t len;
    struct aml_node *result = NULL;
    uint8_t *buffer_buf = NULL;
    
    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &buffer_buf);

    struct aml_ctx pkg_ctx;
    pkg_ctx.aml_data = buffer_buf;
    pkg_ctx.aml_len = len;
    pkg_ctx.current_pos = 0;

    struct aml_node *buffer_size_node;
    rc = aml_parse_next_node(&pkg_ctx, &buffer_size_node);
    if (rc != 0)
    {
        printk("ACPI: BufferOp: Error reading BufferSize node (%i)\n", rc);
        return NULL;
    }

    if (buffer_size_node->type != INTEGER)
    {
        printk("ACPI: BufferOp: BufferSize node has incorrect type (%i)\n", buffer_size_node->type);
        return NULL;
    }

    // Формируем node для ответа
    result = aml_make_node(BUFFER);
    result->buffer.len = buffer_size_node->int_value;
    result->buffer.buffer = kmalloc(result->buffer.len);
    aml_ctx_copy_bytes(&pkg_ctx, result->buffer.buffer, result->buffer.len);

    return result;
}

int aml_op_device(struct aml_ctx *ctx)
{
    int rc = 0;
    size_t len;
    uint8_t *buffer_buf = NULL;
    
    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &buffer_buf);

    struct aml_ctx dev_ctx;
    dev_ctx.aml_data = buffer_buf;
    dev_ctx.aml_len = len;
    dev_ctx.current_pos = 0;

    struct aml_name_string *device_name = aml_read_name_string(&dev_ctx);

    printk("DEVICE OP %s\n", device_name->segments->seg_s);

    struct aml_node *device_node = aml_make_node(DEVICE);
    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, device_name, device_node);
    if (rc != 0)
    {
        printk("ACPI: DeviceOp: Error adding node to namespace (%i)\n", rc);
        return rc;
    }

    struct ns_node *device_ns_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, device_name);
    dev_ctx.scope = device_ns_node; 

    // Парсим внтренности устройства
    struct aml_node *node;
    while (aml_ctx_get_remain_size(&dev_ctx) > 0)
    {
        rc = aml_parse_next_node(&dev_ctx, &node);
        if (rc != 0)
        {
            printk("ACPI: DeviceOp: Error parsing next node (%i)\n", rc);
            return rc;
        }
    }

    return rc;
}

struct aml_node *aml_op_word(struct aml_ctx *ctx)
{
    uint32_t tmp;
    struct aml_node *node = aml_make_node(INTEGER);
    node->int_value = aml_ctx_get_byte(ctx);
    tmp = aml_ctx_get_byte(ctx);
    node->int_value |= tmp << 8;
    return node;
}

struct aml_node *aml_op_dword(struct aml_ctx *ctx)
{
    uint32_t tmp;
    struct aml_node *node = aml_make_node(INTEGER);
    node->int_value = aml_ctx_get_byte(ctx);
    tmp = aml_ctx_get_byte(ctx);
    node->int_value |= tmp << 8;
    tmp = aml_ctx_get_byte(ctx);
    node->int_value |= tmp << 16;
    tmp = aml_ctx_get_byte(ctx);
    node->int_value |= tmp << 24;
    return node;
}

struct aml_node *aml_op_string(struct aml_ctx *ctx)
{
    // Вычисляем текущий указатель
    char *curptr = ctx->aml_data + ctx->current_pos;
    size_t str_length = strlen(curptr);

    // Формируем новую Node
    struct aml_node *node = aml_make_node(STRING);
    node->string.str = kmalloc(str_length + 1);
    strcpy(node->string.str, curptr);

    // не забываем сдвинуть курсор в контексте
    ctx->current_pos += (str_length + 1);

    return node;
}