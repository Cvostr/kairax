#include "aml.h"
#include "kairax/stdio.h"
#include "kairax/string.h"
#include "mem/kheap.h"

int aml_op_alias(struct aml_ctx *ctx)
{
    int rc = 0;
    struct ns_node *source_node = NULL;
    struct aml_name_string *source_name = aml_read_name_string(ctx);
    struct aml_name_string *target_name = aml_read_name_string(ctx);
    //printk("ALIAS (%s to %s)\n", aml_debug_namestring(source_name), aml_debug_namestring(target_name));

    // Получим node по старому имени
    source_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, source_name);
    if (source_node == NULL || source_node->object == NULL)
    {
        printk("ACPI: AliasOp: Unable to find source node (%s) in namespace\n", aml_debug_namestring(source_name));
        rc = -ENOENT;
        goto exit;
    }

    // Сохраним ту же ноду с другим именем
    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, target_name, source_node->object);
    if (rc != 0)
    {
        printk("ACPI: OpRegionOp: Error adding node to namespace (%i)\n", rc);
        goto exit;
    }

exit:
    KFREE_SAFE(source_name);
    KFREE_SAFE(target_name);
    return rc;
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

struct aml_field_desc {
    char name[4];
    uint32_t len;
    uint64_t offset;
};

// 19.6.47 Field (Declare Field Objects)
int aml_parse_next_field(struct aml_ctx *ctx, struct aml_field_desc *desc, uint64_t *current_offset_bits)
{
    uint8_t cur;
    while (aml_ctx_get_remain_size(ctx) > 0)
    {
        cur = aml_ctx_peek_byte(ctx);
        switch (cur)
        {
            case 0:
                aml_ctx_get_byte(ctx);
                uint32_t offset = aml_read_pkg_len(ctx);
                printk("\tReserved field. Offset %x (%i)\n", offset, offset);
                *current_offset_bits += offset;
                break;
            case 1:
                aml_ctx_get_byte(ctx);
                uint8_t access_type = aml_ctx_get_byte(ctx);
                uint8_t access_attrib = aml_ctx_get_byte(ctx);
                printk("\tAccess field. access_type %i access_attrib %i\n", access_type, access_attrib);
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
                aml_ctx_copy_bytes(ctx, desc->name, 4);
                desc->len = aml_read_pkg_len(ctx);
                printk("\tNamed field %s len %i\n", desc->name, desc->len);
                desc->offset = *current_offset_bits;
                *current_offset_bits += desc->len;
                return 0;
        }
    }

    return -1;
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
        printk("ACPI: FieldOp: OpRegionOp node has incorrect type (%i)\n", opregion_node->object->type);
        res = -EINVAL;
        goto exit;
    }

    // Считаем основные флаги
    uint8_t field_flags = aml_ctx_get_byte(&field_ctx);

    printk("FIELD OP pkg_len %i name '%s' flags %x\n", len, opregion_name->segments->seg_s, field_flags);

    struct aml_field_desc desc;
    uint64_t current_offset = 0;
    // Считать поля
    while (aml_ctx_get_remain_size(&field_ctx) > 0)
    {
        res = aml_parse_next_field(&field_ctx, &desc, &current_offset);
        if (res != 0)
        {
            goto exit;
        }

        // Сформировать node
        struct aml_node *field_node = aml_make_node(FIELD);
        field_node->field.len = desc.len;
        field_node->field.offset = desc.offset;
        field_node->field.opregion = opregion_node->object;
        field_node->field.flags = field_flags; // TODO: учесть флаги полей

        // Добавить поле
        res = acpi_ns_add_named_object1(acpi_get_root_ns(), ctx->scope, desc.name, field_node);
        if (res != 0)
        {
            printk("ACPI: FieldOp: Error adding node to namespace (%i)\n", res);
            kfree(field_node);
            goto exit;
        }
    }

exit:
    KFREE_SAFE(nested);
    KFREE_SAFE(opregion_name);
    return res;
}

int aml_op_index_field(struct aml_ctx *ctx)
{   
    int res;
    struct aml_name_string *index_name;
    struct aml_name_string *data_name;
    // Сделаем копию пакета
    size_t len;
    uint8_t *nested = aml_ctx_dup_from_pkg(ctx, &len);

    struct aml_ctx field_ctx;
    field_ctx.aml_data = nested;
    field_ctx.aml_len = len;
    field_ctx.current_pos = 0;

    // Считаем название Index поля
    index_name = aml_read_name_string(&field_ctx);
    struct ns_node *index_field_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, index_name);
    if (index_field_node == NULL || index_field_node->object == NULL)
    {
        printk("ACPI: IndexFieldOp: Unable to find node %s in namespace\n", index_name->segments->seg_s);
        res = -ENOENT;
        goto exit;
    }
    // Проверим тип
    if (index_field_node->object->type != FIELD)
    {
        printk("ACPI: IndexFieldOp: Field node has incorrect type (%i)\n", index_field_node->object->type);
        res = -EINVAL;
        goto exit;
    }

    // Считаем название поля для записи значения
    data_name = aml_read_name_string(&field_ctx);
    struct ns_node *data_field_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, data_name);
    if (data_field_node == NULL || data_field_node->object == NULL)
    {
        printk("ACPI: IndexFieldOp: Unable to find node %s in namespace\n", data_name->segments->seg_s);
        res = -ENOENT;
        goto exit;
    }
    // Проверим тип
    if (data_field_node->object->type != FIELD)
    {
        printk("ACPI: IndexFieldOp: Field node has incorrect type (%i)\n", data_field_node->object->type);
        res = -EINVAL;
        goto exit;
    }

    // Считаем основные флаги
    uint8_t field_flags = aml_ctx_get_byte(&field_ctx);

    printk("INDEX FIELD OP pkg_len %i index_name '%s' data_name '%s' flags %x\n", 
        len, index_name->segments->seg_s, data_name->segments->seg_s, field_flags);

    struct aml_field_desc desc;
    uint64_t current_offset = 0;

    // Считать поля
    while (aml_ctx_get_remain_size(&field_ctx) > 0)
    {
        res = aml_parse_next_field(&field_ctx, &desc, &current_offset);
        if (res != 0)
        {
            goto exit;
        }
    }

exit:
    KFREE_SAFE(index_name);
    KFREE_SAFE(data_name);
    kfree(nested);
    return res;
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
    pkg_ctx.scope = ctx->scope;

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

int aml_op_processor(struct aml_ctx *ctx)
{
    // Deprecated!!
    // Use of this opcode for ProcessorOp has been deprecated in ACPI 6.4, and is not to be reused
    int rc = 0;
    size_t len;
    uint8_t *buffer_buf = NULL;
    
    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &buffer_buf);

    struct aml_ctx processor_ctx;
    processor_ctx.aml_data = buffer_buf;
    processor_ctx.aml_len = len;
    processor_ctx.current_pos = 0;

    struct aml_name_string *processor_name = aml_read_name_string(&processor_ctx);

    uint8_t processor_id = aml_ctx_get_byte(&processor_ctx);
    // TODO: Остальные поля
    // PblkAddress
    // PblkLength

    printk("PROCESSOR OP '%s'. ProcessorID %i\n", processor_name->segments->seg_s, processor_id);

    // TODO: Может формировать это как устройство?
    // Так как Deprecated
    struct aml_node *processor_node = aml_make_node(DEVICE);
    // Заполнить поля
    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, processor_name, processor_node);
    if (rc != 0)
    {
        printk("ACPI: ProcessorOp: Error adding node to namespace (%i)\n", rc);
    }

    KFREE_SAFE(processor_name)
    return 0;
}

int aml_op_mutex(struct aml_ctx *ctx)
{
    int rc = 0;
    // Считаем имя
    struct aml_name_string *mutex_name = aml_read_name_string(ctx);
    // Считаем флаги
    uint8_t mutex_flags = aml_ctx_get_byte(ctx);

    printk("MUTEX '%s'\n", mutex_name->segments->seg_s);

    struct aml_node *mutex_node = aml_make_node(MUTEX);
    mutex_node->mutex.flags = mutex_flags;
    semaphore_init(&mutex_node->mutex.sem, 1);

    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, mutex_name, mutex_node);
    if (rc != 0)
    {
        printk("ACPI: MutexOp: Error adding node to namespace (%i)\n", rc);
        // clear node?
    }

    KFREE_SAFE(mutex_name)
    return rc;
}

struct aml_node *aml_op_if(struct aml_ctx *ctx)
{
    int rc;
    size_t len;
    uint8_t *buffer_buf = NULL;
    
    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &buffer_buf);

    struct aml_ctx if_ctx;
    if_ctx.aml_data = buffer_buf;
    if_ctx.aml_len = len;
    if_ctx.current_pos = 0;
    if_ctx.scope = ctx->scope;

    struct aml_node *predicate;
    rc = aml_parse_next_node(&if_ctx, &predicate);

    printk("IF OP len %i, rc %i\n", len, rc);

    // TODO: Сформировать и сохранить Node

    return NULL;
}

int aml_op_create_byte_field(struct aml_ctx *ctx, size_t field_size)
{
    int res = 0;
    struct aml_node *byte_index_node = NULL;
    struct aml_name_string *source_buffer_name = NULL;
    struct aml_name_string *field_name = NULL;

    // Считаем имя буфера
    source_buffer_name = aml_read_name_string(ctx);

    // Получим буфер по имени
    struct ns_node *src_buffer_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, source_buffer_name);
    if (src_buffer_node == NULL || src_buffer_node->object == NULL)
    {
        printk("ACPI: CreateFieldOp: Unable to find node %s in namespace\n", aml_debug_namestring(source_buffer_name));
        res = -ENOENT;
        goto exit;
    }
    // Проверим тип
    if (src_buffer_node->object->type != BUFFER)
    {
        printk("ACPI: CreateFieldOp: BufferSize node has incorrect type (%i)\n", aml_debug_namestring(source_buffer_name));
        res = -EINVAL;
        goto exit;
    }

    // Получим размер
    res = aml_parse_next_node(ctx, &byte_index_node);
    if (res != 0)
    {
        printk("ACPI: CreateFieldOp: Error reading RegionLen node (%i)\n", res);
        goto exit;
    }

    if (byte_index_node->type != INTEGER)
    {
        printk("ACPI: CreateFieldOp: BufferSize node has incorrect type (%i)\n", byte_index_node->type);
        res = -EINVAL;
        goto exit;
    }

    // получим название поля
    field_name = aml_read_name_string(ctx);

    printk("CreateFieldOp: %s, ByteIndex: %i, FieldName: %s, FieldSize: %i\n",
        aml_debug_namestring(source_buffer_name), byte_index_node->int_value, aml_debug_namestring(field_name), field_size);
    
    // TODO: Сформировать и сохранить Node

exit:
    KFREE_SAFE(byte_index_node)
    KFREE_SAFE(source_buffer_name)
    KFREE_SAFE(field_name)
    return res;
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

struct aml_node *aml_eval_string(struct aml_ctx *ctx)
{
    struct aml_name_string *name = aml_read_name_string(ctx);
    printk("ACPI: String %s\n", aml_debug_namestring(name));

    // Вероятно, это ссылка, попробуем найти объект по этому имени
    struct ns_node *ns_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, name);
    if (ns_node != NULL)
    {
        return ns_node->object;
    }

    printk("ACPI: StringOp: Not found object with name %s\n", aml_debug_namestring(name));

    // TODO: Implement
    return NULL;
}