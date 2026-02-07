#include "aml.h"
#include "kairax/stdio.h"
#include "kairax/string.h"
#include "mem/kheap.h"

//#define AML_DEBUG_NAMEOP
//#define AML_DEBUG_NAMED_FIELD
//#define AML_DEBUG_RESERVED_FIELD
//#define AML_DEBUG_ALIAS
#define AML_DEBUG_SCOPE
#define AML_DEBUG_UNKNOWN_STRINGS
//#define AML_DEBUG_OP_REGION
//#define AML_DEBUG_METHOD
//#define AML_DEBUG_DEVICE
//#define AML_OP_BINARY_OP

int aml_op_external_decl(struct aml_ctx *ctx)
{
    printk("ACPI: ExternalOp not implemented!\n");
    return -ENOTSUP;
}

int aml_op_alias(struct aml_ctx *ctx)
{
    int rc = 0;
    struct ns_node *source_node = NULL;
    struct aml_name_string *source_name = aml_read_name_string(ctx);
    struct aml_name_string *target_name = aml_read_name_string(ctx);

#ifdef AML_DEBUG_ALIAS
    printk("ALIAS (%s to %s)\n", aml_debug_namestring(source_name), aml_debug_namestring(target_name));
#endif

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
#ifdef AML_DEBUG_SCOPE
    printk("SCOPE pkg_len %i, name %s (root=%i, base=%i, segments=%i)\n", 
        len, aml_debug_namestring(ns_name), ns_name->from_root, ns_name->base, ns_name->segments_num);
#endif

    // Попробуем получить объект по считанному имени
    struct ns_node *scope_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, ns_name);
    if (scope_node == NULL)
    {
        printk("ACPI: ScopeOp: Unable to find scope node (%s) in namespace\n", aml_debug_namestring(ns_name));
        rc = -ENOENT;
        goto exit;
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

exit:
    KFREE_SAFE(ns_name);
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
        
#ifdef AML_DEBUG_OP_REGION
    printk("OP REGION OP name %s reg space 0x%x\n", region_name->segments->seg_s, region_space);
#endif

    uint64_t region_offset, region_len;

    rc = aml_parse_next_node(ctx, &region_offset_node);
    if (rc != 0)
    {
        printk("ACPI: OpRegionOp: Error reading RegionOffset node (%i)\n", rc);
        goto exit;
    }

    rc = aml_node_as_integer(region_offset_node, &region_offset);
    if (rc != 0)
    {
        printk("ACPI: OpRegionOp: Error casting RegionOffset node to Integer (%i)\n", rc);
        goto exit;
    }
    
    rc = aml_parse_next_node(ctx, &region_len_node);
    if (rc != 0)
    {
        printk("ACPI: OpRegionOp: Error reading RegionLen node (%i)\n", rc);
        goto exit;
    }

    rc = aml_node_as_integer(region_len_node, &region_len);
    if (rc != 0)
    {
        printk("ACPI: OpRegionOp: Error casting RegionLen node to Integer (%i)\n", rc);
        goto exit;
    }

    struct aml_node *region_node = aml_make_node(OP_REGION);
    region_node->op_region.space = region_space;
    region_node->op_region.offset = region_offset;
    region_node->op_region.len = region_len;
    region_node->op_region.scope = ctx->scope;

    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, region_name, region_node);
    if (rc != 0)
    {
        printk("ACPI: OpRegionOp: Error adding node to namespace (%i)\n", rc);
        goto exit;
    }

#ifdef AML_DEBUG_OP_REGION
    printk("\toffset type %i value 0x%x\n", region_offset_node->type, region_offset);
    printk("\tlen type %i len 0x%x\n", region_len_node->type, region_len);
#endif

exit:
    aml_free_node(region_len_node);
    aml_free_node(region_offset_node);
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
#ifdef AML_DEBUG_RESERVED_FIELD
                printk("\tReserved field. Offset %x (%i)\n", offset, offset);
#endif
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
#ifdef AML_DEBUG_NAMED_FIELD
                printk("\tNamed field %s len %i\n", desc->name, desc->len);
#endif
                desc->offset = *current_offset_bits;
                *current_offset_bits += desc->len;
                return 0;
        }
    }

    // Дошли до конца, но поле так и не встретили.
    // Ну такое тоже бывает
    return -ENOENT;
}

// 20.2. AML Grammar Definition (998)
int aml_op_field(struct aml_ctx *ctx)
{
    int res = 0;
    struct aml_name_string *opregion_name = NULL;
    // Получим длину данных, указатель на начало и сместим курсор
    size_t len;
    uint8_t *nested = 0;
    len = aml_ctx_addr_from_pkg(ctx, &nested);
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
        if (res == -ENOENT)
        {
            // Некоторые "гиганты мысли" любят оставлять в конце например Offset(0x4) без поля
            // Это нормально, поэтому ошибки нет
            printk("ACPI: FieldOp: Trailing elements without field\n");
            res = 0;
            break;
        }
        else if (res != 0)
        {
            printk("ACPI: FieldOp: Error reading field declaration (%i)\n", res);
            goto exit;
        }

        // Сформировать node
        struct aml_node *field_node = aml_make_node(FIELD);
        field_node->field.type = DEFAULT;
        field_node->field.len = desc.len;
        field_node->field.offset = desc.offset;
        field_node->field.flags = field_flags; // TODO: учесть флаги полей
        field_node->field.mem.opregion = opregion_node->object;

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
    KFREE_SAFE(opregion_name);
    return res;
}

int aml_op_index_field(struct aml_ctx *ctx)
{   
    int res;
    struct aml_name_string *index_name;
    struct aml_name_string *data_name;
    // Получим длину данных, указатель на начало и сместим курсор
    size_t len;
    uint8_t *nested = 0;
    len = aml_ctx_addr_from_pkg(ctx, &nested);

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
        if (res == -ENOENT)
        {
            // Некоторые "гиганты мысли" любят оставлять в конце например Offset(0x4) без поля
            // Это нормально, поэтому ошибки нет
            printk("ACPI: IndexFieldOp: Trailing elements without field\n");
            res = 0;
            break;
        }
        else if (res != 0)
        {
            printk("ACPI: IndexFieldOp: Error reading field declaration (%i)\n", res);
            goto exit;
        }

        // Сформировать node
        struct aml_node *field_node = aml_make_node(FIELD);
        field_node->field.type = INDEX;
        field_node->field.len = desc.len;
        field_node->field.offset = desc.offset;
        field_node->field.flags = field_flags; // TODO: учесть флаги полей
        field_node->field.mem.index.index = index_field_node->object;
        field_node->field.mem.index.data = data_field_node->object;

        // Добавить поле
        res = acpi_ns_add_named_object1(acpi_get_root_ns(), ctx->scope, desc.name, field_node);
        if (res != 0)
        {
            printk("ACPI: IndexFieldOp: Error adding node to namespace (%i)\n", res);
            kfree(field_node);
            goto exit;
        }
    }

exit:
    KFREE_SAFE(index_name);
    KFREE_SAFE(data_name);
    return res;
}

int aml_op_method(struct aml_ctx *ctx)
{
    int res = 0;
    size_t len;
    uint8_t *nested = NULL;
    
    struct aml_name_string *method_name = NULL;

    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &nested);

    struct aml_ctx method_ctx;
    method_ctx.aml_data = nested;
    method_ctx.aml_len = len;
    method_ctx.current_pos = 0;
    method_ctx.scope = ctx->scope;

    // Считаем название метода
    method_name = aml_read_name_string(&method_ctx);

    uint8_t method_flags = aml_ctx_get_byte(&method_ctx);
    uint8_t arg_count = method_flags & 0b111;
    uint8_t serialize = (method_flags > 3) & 1;
    uint8_t sync_level = (method_flags > 4) & 0xF;

#ifdef AML_DEBUG_METHOD
    printk("METHOD OP pkg_len %i name '%s' args %i, serialized %i, sync level %i\n", 
        len, method_name->segments->seg_s, arg_count, serialize, sync_level);
#endif

    // Формируем node
    struct aml_node *node = aml_make_node(METHOD);
    node->method.args = arg_count;
    node->method.serialized = serialize;
    node->method.sync_level = sync_level;
    node->method.code = method_ctx.aml_data + method_ctx.current_pos;
    node->method.code_size = aml_ctx_get_remain_size(&method_ctx);

    // Добавить поле
    res = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, method_name, node);
    if (res != 0)
    {
        printk("ACPI: MethodOp: Error adding node to namespace (%i)\n", res);
        kfree(node);
        goto exit;
    }

exit:
    KFREE_SAFE(method_name);
    return res;
}

int aml_op_name(struct aml_ctx *ctx)
{
    int rc = 0;
    struct aml_name_string *name = NULL;
    // Считаем имя
    name = aml_read_name_string(ctx);
#ifdef AML_DEBUG_NAMEOP
    printk("NameOp: '%s'\n", name->segments->seg_s);
#endif

    // Распарсим ноду
    struct aml_node *value;
    rc = aml_parse_next_node(ctx, &value);
    if (rc != 0)
    {
        printk("ACPI: NameOp: Error reading next node (%i)\n", rc);
        goto exit;
    }

    if (value == NULL)
    {
        printk("ACPI: NameOp: next node is NULL\n");
        rc = -EINVAL;
        goto exit;
    }

    // Добавляем считанную ноду с именем
    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, name, value);
    if (rc != 0)
    {
        printk("ACPI: NameOp: Error adding node to namespace (%i)\n", rc);
        goto exit;
    }

exit:
    KFREE_SAFE(name);
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

    aml_acquire_node(node);
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

    aml_acquire_node(result);
    return result;
}

int aml_op_device(struct aml_ctx *ctx)
{
    int rc = 0;
    size_t len;
    uint8_t *buffer_buf = NULL;
    struct aml_name_string *device_name = NULL;
    
    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &buffer_buf);

    struct aml_ctx dev_ctx;
    dev_ctx.aml_data = buffer_buf;
    dev_ctx.aml_len = len;
    dev_ctx.current_pos = 0;

    // Считаем имя устройства
    device_name = aml_read_name_string(&dev_ctx);

#ifdef AML_DEBUG_DEVICE
    printk("DEVICE OP %s\n", device_name->segments->seg_s);
#endif

    struct aml_node *device_node = aml_make_node(DEVICE);
    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, device_name, device_node);
    if (rc != 0)
    {
        printk("ACPI: DeviceOp: Error adding node to namespace (%i)\n", rc);
        goto exit;
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
            goto exit;
        }
    }

exit:
    KFREE_SAFE(device_name);
    return rc;
}

int aml_op_thermal_zone(struct aml_ctx *ctx)
{
    int rc = 0;
    size_t len;
    uint8_t *buffer_buf = NULL;
    struct aml_name_string *zone_name = NULL;
    
    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &buffer_buf);

    struct aml_ctx zone_ctx;
    zone_ctx.aml_data = buffer_buf;
    zone_ctx.aml_len = len;
    zone_ctx.current_pos = 0;

    // Считаем имя устройства
    zone_name = aml_read_name_string(&zone_ctx);

    printk("THERMAL ZONE OP %s\n", zone_name->segments->seg_s);

    struct aml_node *thermal_zone_node = aml_make_node(THERMAL_ZONE);
    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, zone_name, thermal_zone_node);
    if (rc != 0)
    {
        printk("ACPI: ThermalZoneOp: Error adding node to namespace (%i)\n", rc);
        goto exit;
    }

    struct ns_node *zone_ns_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, zone_name);
    zone_ctx.scope = zone_ns_node; 

    // Парсим внтренности устройства
    struct aml_node *node;
    while (aml_ctx_get_remain_size(&zone_ctx) > 0)
    {
        rc = aml_parse_next_node(&zone_ctx, &node);
        if (rc != 0)
        {
            printk("ACPI: ThermalZoneOp: Error parsing next node (%i)\n", rc);
            goto exit;
        }
    }

exit:
    KFREE_SAFE(zone_name);
    return rc;
}

int aml_op_power_resource(struct aml_ctx *ctx)
{
    int rc = 0;
    size_t len;
    uint8_t *buffer_buf = NULL;
    struct aml_name_string *pwr_res_name = NULL;
    
    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &buffer_buf);

    struct aml_ctx pwr_ctx;
    pwr_ctx.aml_data = buffer_buf;
    pwr_ctx.aml_len = len;
    pwr_ctx.current_pos = 0;

    // Считаем основные аттрибуты
    pwr_res_name = aml_read_name_string(&pwr_ctx);
    uint8_t system_level = aml_ctx_get_byte(&pwr_ctx);
    uint16_t resource_order = aml_ctx_get_word(&pwr_ctx);

    printk("POWER RESOURCE OP %s, SystemLevel %i, ResourceOrder %i\n", 
        pwr_res_name->segments->seg_s, system_level, resource_order);

    // Создать Node
    struct aml_node *pwr_res_node = aml_make_node(POWER_RESOURCE);
    pwr_res_node->power_resource.system_level = system_level;
    pwr_res_node->power_resource.resource_order = resource_order;
    // Добавить Node
    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, pwr_res_name, pwr_res_node);
    if (rc != 0)
    {
        printk("ACPI: PowerResOp: Error adding node to namespace (%i)\n", rc);
        goto exit;
    }

    struct ns_node *device_ns_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, pwr_res_name);
    pwr_ctx.scope = device_ns_node; 

    // Парсим внутренности
    struct aml_node *node;
    while (aml_ctx_get_remain_size(&pwr_ctx) > 0)
    {
        rc = aml_parse_next_node(&pwr_ctx, &node);
        if (rc != 0)
        {
            printk("ACPI: PowerResOp: Error parsing next node (%i)\n", rc);
            goto exit;
        }
    }

exit:
    KFREE_SAFE(pwr_res_name);
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

    // Считаем основные заголовочные поля
    uint8_t processor_id = aml_ctx_get_byte(&processor_ctx);
    uint32_t p_blk_address = aml_ctx_get_dword(&processor_ctx);
    uint8_t p_blk_len = aml_ctx_get_byte(&processor_ctx);

    printk("PROCESSOR OP '%s'. ProcessorID %i, PblkAddress 0x%p, PblkLen %i\n", 
        processor_name->segments->seg_s, processor_id, p_blk_address, p_blk_len);

    // TODO: Может формировать это как устройство?
    // Так как Deprecated
    struct aml_node *processor_node = aml_make_node(DEVICE);
    // Заполнить поля
    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, processor_name, processor_node);
    if (rc != 0)
    {
        printk("ACPI: ProcessorOp: Error adding node to namespace (%i)\n", rc);
    }

    // TODO: Считать вложенные Node

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
        aml_free_node(mutex_node);
    }

    KFREE_SAFE(mutex_name)
    return rc;
}

int aml_op_event(struct aml_ctx *ctx)
{
    int rc = 0;
    // Считаем имя
    struct aml_name_string *event_name = aml_read_name_string(ctx);

    printk("EVENT '%s'\n", event_name->segments->seg_s);

    struct aml_node *event_node = aml_make_node(EVENT);
    event_node->event.sigcount.counter = 0;

    rc = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, event_name, event_node);
    if (rc != 0)
    {
        printk("ACPI: EventOp: Error adding node to namespace (%i)\n", rc);
        aml_free_node(event_node);
    }

    KFREE_SAFE(event_name)
    return rc;
}

int aml_op_if(struct aml_ctx *ctx, struct aml_node **returned_node)
{
    int rc;
    size_t len = 0, else_len = 0;
    uint8_t *buffer_buf = NULL, *else_buf = NULL;
    struct aml_node *predicate = NULL;
    
    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &buffer_buf);
    
    // Смотрим, есть ли дальше ELSE
    uint8_t else_test = aml_ctx_peek_byte(ctx);
    if (else_test == AML_OP_ELSE)
    {
        // Дальше есть ELSE
        // Вытащить ElseOp
        else_test = aml_ctx_get_byte(ctx);
        // Получить длину блока ELSE и сместим курсор
        else_len = aml_ctx_addr_from_pkg(ctx, &else_buf);
    }

    // Читаем предикат и действия, которые будут выполнены при ИСТИНА
    struct aml_ctx if_ctx;
    memset(&if_ctx, 0, sizeof(struct aml_ctx));
    if_ctx.aml_data = buffer_buf;
    if_ctx.aml_len = len;
    if_ctx.current_pos = 0;
    aml_ctx_inherit(ctx, &if_ctx);

    // Считываем предикат
    rc = aml_parse_next_node(&if_ctx, &predicate);
    if (rc != 0)
    {
        printk("ACPI: IfOp: Error parsing predicate (%i)\n", rc);
        goto exit;
    }
    if (predicate == NULL)
    {
        printk("ACPI: IfOp: Predicate is NULL\n");
        goto exit;
    }
    if (predicate->type != INTEGER)
    {
        printk("ACPI: IfOp: Predicate is not Integer\n");
        goto exit;
    }

    printk("IF OP predicate value %i\n", predicate->int_value);

    struct aml_node *node;

    if (predicate->int_value == AML_FALSE)
    {
        if (else_len > 0)
        {
            // Если ЛОЖЬ и у нас есть код для ELSE, то вероломно подменяем данные в контексте
            if_ctx.aml_data = else_buf;
            if_ctx.aml_len = else_len;
            // не забываем сбросить позицию
            if_ctx.current_pos = 0;
            printk("ACPI: IfOp: Executing ELSE Branch\n");
        }
        else
        {
            // нечего выполнять - просто выходим
            goto exit;
        }
    }

    // Парсим все, что есть внутри
    while (aml_ctx_get_remain_size(&if_ctx) > 0)
    {
        rc = aml_parse_next_node(&if_ctx, &node);
        if (rc == (0xFF00 | AML_OP_RETURN))
        {
            // Выход из метода
            *returned_node = node;
            // проброс rc дальше 
            break;
        }
        else if (rc != 0)
        {
            printk("ACPI: IfOp: Error parsing next node (%i)\n", rc);
            return rc;
        }
    }

exit:
    KFREE_SAFE(predicate);
    return rc;
}

int aml_op_while(struct aml_ctx *ctx, struct aml_node **returned_node)
{
    int rc;
    size_t len = 0;
    uint8_t *buffer_buf = NULL;
    struct aml_node *predicate = NULL;
    uint64_t predicate_int;
    struct aml_ctx while_ctx;
    int interrupted = FALSE;
    struct aml_node *node;

    // Получим длину данных, указатель на начало и сместим курсор
    len = aml_ctx_addr_from_pkg(ctx, &buffer_buf);

    while (interrupted == FALSE)
    {
        // На каждом раунде сбрасываем контекст
        memset(&while_ctx, 0, sizeof(struct aml_ctx));
        while_ctx.aml_data = buffer_buf;
        while_ctx.aml_len = len;
        while_ctx.current_pos = 0;
        aml_ctx_inherit(ctx, &while_ctx);

        // Считываем предикат
        rc = aml_parse_next_node(&while_ctx, &predicate);
        if (rc != 0)
        {
            printk("ACPI: WhileOp: Error parsing predicate (%i)\n", rc);
            interrupted = TRUE;
            goto end_loop;
        }
        if (predicate == NULL)
        {
            printk("ACPI: WhileOp: Predicate is NULL\n");
            interrupted = TRUE;
            goto end_loop;
        }
        // Пробуем сконвертировать предикат в Integer
        rc = aml_to_uint64(predicate, &predicate_int);
        if (rc != 0)
        {
            printk("ACPI: WhileOp: Error converting Predicate to int (%i)\n", rc);
            interrupted = TRUE;
            goto end_loop;
        }

        printk("WHILE OP Round predicate value %i\n", predicate_int);

        if (predicate_int != AML_FALSE)
        {
            // Парсим все, что есть внутри
            while (aml_ctx_get_remain_size(&while_ctx) > 0)
            {
                rc = aml_parse_next_node(&while_ctx, &node);
                if (rc == (0xFF00 | AML_OP_RETURN))
                {
                    interrupted = TRUE;
                    // Выход из метода
                    *returned_node = node;
                    // проброс rc дальше 
                    break;
                }
                else if (rc == (0xFF00 | AML_OP_BREAK))
                {
                    // Перебиваем код возврата на успешный
                    rc = 0;
                    // Больше ничего не делаем
                    interrupted = TRUE;
                    break;
                }
                else if (rc == (0xFF00 | AML_OP_CONTINUE))
                {
                    break;
                }
                else if (rc != 0)
                {
                    printk("ACPI: WhileOp: Error parsing next node (%i)\n", rc);
                    return rc;
                }
            }
        }

    end_loop:
        aml_free_node(predicate);
    }

    return rc;
}

int aml_op_create_buffer_field(struct aml_ctx *ctx, size_t field_size)
{
    int res = 0;
    struct aml_node *index_node = NULL;
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
        printk("ACPI: CreateFieldOp: SourceBufferName node has incorrect type (%i)\n", aml_debug_namestring(source_buffer_name));
        res = -EINVAL;
        goto exit;
    }

    // Получим индекс
    res = aml_parse_next_node(ctx, &index_node);
    if (res != 0)
    {
        printk("ACPI: CreateFieldOp: Error reading ByteIndex node (%i)\n", res);
        goto exit;
    }
    // Проверим тип
    if (index_node->type != INTEGER)
    {
        printk("ACPI: CreateFieldOp: Index node has incorrect type (%i)\n", index_node->type);
        res = -EINVAL;
        goto exit;
    }

    // Получим и преобразуем значение Index
    size_t index_value = index_node->int_value;
    index_value *= 8; // перевод в биты

    // получим название поля
    field_name = aml_read_name_string(ctx);

    printk("CreateFieldOp: %s, Index: %i, FieldName: %s, FieldSize: %i\n",
        aml_debug_namestring(source_buffer_name), index_value, aml_debug_namestring(field_name), field_size);

    // Сформировать node
    struct aml_node *field_node = aml_make_node(BUFFER_FIELD);
    field_node->buffer_field.buffer = src_buffer_node->object;
    field_node->buffer_field.index_bits = index_value;
    field_node->buffer_field.size_bits = field_size;

    // Добавить поле
    res = acpi_ns_add_named_object(acpi_get_root_ns(), ctx->scope, field_name, field_node);
    if (res != 0)
    {
        printk("ACPI: CreateFieldOp: Error adding node to namespace (%i)\n", res);
        aml_free_node(field_node);
        goto exit;
    }


exit:
    aml_free_node(index_node);
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
    aml_acquire_node(node);
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
    aml_acquire_node(node);
    return node;
}

struct aml_node *aml_op_qword(struct aml_ctx *ctx)
{
    struct aml_node *node = aml_make_node(INTEGER);
    node->int_value = aml_ctx_get_qword(ctx);
    aml_acquire_node(node);
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
    aml_acquire_node(node);

    // не забываем сдвинуть курсор в контексте
    ctx->current_pos += (str_length + 1);

    return node;
}

int aml_eval_string(struct aml_ctx *ctx, struct aml_node **out)
{
    int rc = 0;
    struct aml_node *result = NULL;
    struct aml_name_string *name = aml_read_name_string(ctx);
    printk("ACPI: String %s\n", aml_debug_namestring(name));

    // Вероятно, это ссылка, попробуем найти объект по этому имени
    struct ns_node *ns_node = acpi_ns_get_node(acpi_get_root_ns(), ctx->scope, name);
    if (ns_node != NULL)
    {
        result = ns_node->object;
        if (result->type == METHOD)
        {
            // Если тип объекта, на который ссылаемся - метод, значит это вызов метода
            // Выполняем из scope - сам метод. Вроде как надо из scope родителя
            // Но ломаются некоторые методы
            // Возможно это одно из нарушений стандарта, которое прижилось
            rc = aml_execute_method(ctx, result, ns_node, TRUE, &result);
        }
        else
        {
            // Увеличим счетчик ссылок, т.к мы вытащили объект из namespace
            aml_acquire_node(result);
        }

        *out = result;
        goto exit;
    }
#ifdef AML_DEBUG_UNKNOWN_STRINGS
    printk("ACPI: StringOp: Not found object with name %s\n", aml_debug_namestring(name));
#endif

    // TODO: Implement
exit:
    kfree(name);
    return rc;
}

int aml_op_return(struct aml_ctx *ctx, struct aml_node **out)
{
    int rc = aml_parse_next_node(ctx, out);
    if (rc != 0)
    {
        printk("ACPI: ReturnOp: Error reading next node (%i)\n", rc);
        return rc;
    }

    // Возвращаем специальный код ошибки
    return (0xFF00 | AML_OP_RETURN);
}

int aml_op_local(struct aml_ctx *ctx, uint8_t index, struct aml_node **out)
{
    if (index > AML_LOCALS_NUM)
    {
        return -EINVAL;
    }

    struct aml_node *local_node = ctx->locals[index];
    if (local_node == NULL)
    {
        printk("ACPI: Local%i is not initialized\n", index);
        return -ENOENT;
    }

    aml_acquire_node(local_node);
    *out = local_node;

    return 0;
}

int aml_op_arg(struct aml_ctx *ctx, uint8_t index, struct aml_node **out)
{
    if (index > AML_ARGS_NUM)
    {
        return -EINVAL;
    }

    struct aml_node *arg_node = ctx->args[index];
    if (arg_node == NULL)
    {
        printk("ACPI: Arg%i is not initialized\n", index);
        return -ENOENT;
    }

    aml_acquire_node(arg_node);
    *out = arg_node;

    return 0;
}

int aml_op_logical_not(struct aml_ctx *ctx, struct aml_node** node_out)
{
    int res;
    struct aml_node *operand = NULL;
    uint64_t operand_value;

    // Считать node
    // Очень удобно сделано >=, <=
    res = aml_parse_next_node(ctx, &operand);
    if (res != 0)
    {
        printk("ACPI: LnotOp: Error reading operand node (%i)\n", res);
        goto exit;
    }

    // Попытаться сконвертировать в Integer
    res = aml_node_as_integer(operand, &operand_value);
    if (res != 0)
    {
        printk("ACPI: LnotOp: Error casting operand to Integer (%i)\n", res);
        goto exit;
    }

    // Инвертировать
    operand_value = (operand_value > 0) ? AML_FALSE : AML_TRUE;
    
    // Сформировать node с результатом
    struct aml_node *result_node = aml_make_node(INTEGER);
    result_node->int_value = operand_value;
    // Записать
    aml_acquire_node(result_node);
    *node_out = result_node;

exit:
    aml_free_node(operand);
    return res;
}

int aml_op_binary(struct aml_ctx *ctx, uint8_t opcode, struct aml_node** node_out)
{
    int res;
    struct aml_node *operand1 = NULL, *operand2 = NULL;
    uint64_t op1_value, op2_value;

    res = aml_parse_next_node(ctx, &operand1);
    if (res != 0)
    {
        printk("ACPI: BinaryOp: Error reading operand 1 node (%i)\n", res);
        goto exit;
    }

    res = aml_parse_next_node(ctx, &operand2);
    if (res != 0)
    {
        printk("ACPI: BinaryOp: Error reading operand 2 node (%i)\n", res);
        goto exit;
    }

    res = aml_node_as_integer(operand1, &op1_value);
    if (res != 0)
    {
        printk("ACPI: BinaryOp: Error casting operand 1 to Integer (%i)\n", res);
        goto exit;
    }

    res = aml_node_as_integer(operand2, &op2_value);
    if (res != 0)
    {
        printk("ACPI: BinaryOp: Error casting operand 2 to Integer (%i)\n", res);
        goto exit;
    }

    // проверка деления на 0
    if ((opcode == AML_OP_DIVIDE || opcode == AML_OP_MOD) && op2_value == 0)
    {
        printk("ACPI: Division by zero!\n");
        return -EINVAL;
    }

    uint64_t result;
    switch (opcode)
    {
    case AML_OP_ADD:
        result = op1_value + op2_value;
        break;
    case AML_OP_SUBTRACT:
        result = op1_value - op2_value;
        break;
    case AML_OP_MULTIPLY:
        result = op1_value * op2_value;
        break;
    case AML_OP_DIVIDE:
        result = op1_value / op2_value;
        // TODO: остаток?
        break;
    case AML_OP_AND:
        result = op1_value & op2_value;
        break;
    case AML_OP_OR:
        result = op1_value | op2_value;
        break;
    case AML_OP_NAND:
        result = ~(op1_value & op2_value);
        break;
    case AML_OP_NOR:
        result = ~(op1_value | op2_value);
        break;
    case AML_OP_XOR:
        result = (op1_value ^ op2_value);
        break;
    case AML_OP_MOD:
        result = (op1_value % op2_value);
        break;
    case AML_OP_SHIFT_LEFT:
        result = op1_value << op2_value;
        break;
    case AML_OP_SHIFT_RIGHT:
        result = op1_value >> op2_value;
        break;
    default:
        printk("ACPI: BinaryOp: Unknown operation (%i)\n", opcode);
        res = -EINVAL;
        goto exit;
    }

#ifdef AML_OP_BINARY_OP
    printk("ACPI: BinaryOp: Op1 (type=%i, value=%i) Op2 (type=%i, value=%i). Result %i\n", 
        operand1->type, op1_value, operand2->type, op2_value, result);
#endif

    // Сформировать node с результатом
    struct aml_node *result_node = aml_make_node(INTEGER);
    result_node->int_value = result;

    // Сохранить в target
    res = aml_store_to_target(ctx, result_node);
    if (res != 0)
    {
        printk("ACPI: BinaryOp: Error writing to target %i!\n", res);
        goto exit;
    }

    // положить в ответ
    aml_acquire_node(result_node);
    *node_out = result_node;

exit:
    aml_free_node(operand1);
    aml_free_node(operand2);
    return res;
}

int aml_op_to_integer(struct aml_ctx *ctx, struct aml_node** node_out)
{
    int res;
    struct aml_node *value_node = NULL;
    uint64_t int_val;

    res = aml_parse_next_node(ctx, &value_node);
    if (res != 0)
    {
        printk("ACPI: ToInteger: Error reading operand node (%i)\n", res);
        goto exit;
    }

    if (value_node->type == INTEGER)
    {
        // Счетчик увеличивать не будем, потому что не будем уменьшать
        *node_out = value_node;
        return 0;
    }

    res = aml_to_uint64(value_node, &int_val);
    if (res != 0)
    {
        printk("ACPI: ToInteger: Error converting operand to int (%i)\n", res);
        goto exit;
    }

    // Сформировать node с результатом
    struct aml_node *result_node = aml_make_node(INTEGER);
    result_node->int_value = int_val;
    // Отдать наверх с увеличением счетчика
    aml_acquire_node(result_node);
    *node_out = result_node;

    // Сохранить в target
    res = aml_store_to_target(ctx, result_node);
    if (res != 0)
    {
        printk("ACPI: ToInteger: Error writing to target %i!\n", res);
        goto exit;
    }

exit:
    aml_free_node(value_node);
    return res;
}

int aml_op_ilogical(struct aml_ctx *ctx, uint8_t opcode, struct aml_node** node_out)
{
    int res;
    struct aml_node *operand1 = NULL, *operand2 = NULL;
    uint64_t op1_ival, op2_ival;
    uint64_t cmpresult;

    // Чтение операндовы
    res = aml_parse_next_node(ctx, &operand1);
    if (res != 0)
    {
        printk("ACPI: ILogical: Error reading operand 1 node (%i)\n", res);
        goto exit;
    }

    res = aml_parse_next_node(ctx, &operand2);
    if (res != 0)
    {
        printk("ACPI: ILogical: Error reading operand 2 node (%i)\n", res);
        goto exit;
    }

    // Приведение операндов к Integer
    res = aml_node_as_integer(operand1, &op1_ival);
    if (res != 0)
    {
        printk("ACPI: ILogical: Error casting Op1 to int (%i)\n", res);
        goto exit;
    }

    // Пробуем сконвертировать второй операнд в Integer
    res = aml_to_uint64(operand2, &op2_ival);
    if (res != 0)
    {
        printk("ACPI: ILogical: Error converting Op1 to int (%i)\n", res);
        goto exit;
    }

    switch (opcode)
    {
    case AML_OP_LOR:
        cmpresult = (op1_ival || op2_ival);
        break;
    case AML_OP_LAND:
        cmpresult = (op1_ival && op2_ival);
        break;
    default:
        printk("ACPI: ILogical: Unknown opcode %i\n", opcode);
        res = -EINVAL;
        break;
    }

    // Сформировать node с результатом
    struct aml_node *result_node = aml_make_node(INTEGER);
    // По спеке, когда истинно, то ожидается 0xFFFFFFFF
    result_node->int_value = (cmpresult == TRUE) ? AML_TRUE : AML_FALSE;
    // Сохраним
    aml_acquire_node(result_node);
    *node_out = result_node;

exit:
    aml_free_node(operand1);
    aml_free_node(operand2);
    return res;
}

int aml_op_compare(struct aml_ctx *ctx, uint8_t opcode, struct aml_node** node_out)
{
    int res;
    struct aml_node *operand1 = NULL, *operand2 = NULL;
    uint64_t cmpresult;

    res = aml_parse_next_node(ctx, &operand1);
    if (res != 0)
    {
        printk("ACPI: BinaryOp: Error reading operand 1 node (%i)\n", res);
        goto exit;
    }

    if (operand1 == NULL)
    {
        res = -ENOENT;
        printk("ACPI: BinaryOp: Operand 1 Node is NULL\n");
        goto exit;
    }

    res = aml_parse_next_node(ctx, &operand2);
    if (res != 0)
    {
        printk("ACPI: BinaryOp: Error reading operand 2 node (%i)\n", res);
        goto exit;
    }

    if (operand2 == NULL)
    {
        res = -ENOENT;
        printk("ACPI: BinaryOp: Operand 2 Node is NULL\n");
        goto exit;
    }

    // Смотрим тип первого операнда
    switch (operand1->type)
    {
    case STRING:
        printk("ACPI: Strings comparation is not supported\n");
        res = -ENOSYS;
        goto exit;
        break;
    case BUFFER:
        printk("ACPI: Buffers comparation is not supported\n");
        res = -ENOSYS;
        goto exit;
        break;
    default:
        // Вероятно, это Integer
        uint64_t op1_ival, op2_ival;
        res = aml_node_as_integer(operand1, &op1_ival);
        if (res != 0)
        {
            printk("ACPI: CompareOp: Error casting Op1 to int (%i)\n", res);
            goto exit;
        }

        // Пробуем сконвертировать второй операнд в Integer
        res = aml_to_uint64(operand2, &op2_ival);
        if (res != 0)
        {
            printk("ACPI: CompareOp: Error converting Op1 to int (%i)\n", res);
            goto exit;
        }

        printk("ACPI: CompareOp: Op1 : %i, Op2: %i\n", op1_ival, op2_ival);

        switch (opcode)
        {
            case AML_OP_LEQUAL:
                cmpresult = op1_ival == op2_ival;
                break;
            case AML_OP_LGREATER:
                cmpresult = op1_ival > op2_ival;
                break;
            case AML_OP_LLESS:
                cmpresult = op1_ival < op2_ival;
                break;
            default:
                // Я заебался
        }
    
        break;
    }

    // Сформировать node с результатом
    struct aml_node *result_node = aml_make_node(INTEGER);
    // По спеке, когда истинно, то ожидается 0xFFFFFFFF
    result_node->int_value = (cmpresult == TRUE) ? AML_TRUE : AML_FALSE;
    // Сохраним
    aml_acquire_node(result_node);
    *node_out = result_node;

exit:
    aml_free_node(operand1);
    aml_free_node(operand2);
    return res;
}

int aml_op_cond_ref_of(struct aml_ctx *ctx, struct aml_node **out)
{
    int res;
    int src_exists = TRUE;
    struct aml_node *src_node = NULL;
    struct aml_node *result_node = NULL;
    struct aml_node *ref_node = NULL;

    // Считаем ссылку на node из SuperName
    res = aml_read_supername_as_ref(ctx, &src_node);
    if (res == -ENOENT)
    {
        printk("ACPI: CondRefOfOp: Can't find node\n");
        // помечаем, что SOURCE не существует
        src_exists = FALSE;
        return 0;
    }
    else if (res != 0)
    {
        printk("ACPI: CondRefOfOp: Error reading SuperName: %i\n", res);
        return res;
    }

    if (src_exists == TRUE)
    {
        // Если источник существует
        // Создадим ссылку с увеличением счетчика ссылок на источник
        ref_node = aml_make_node(REFERENCE);
        aml_acquire_node(ref_node);
        ref_node->link = src_node;
        aml_acquire_node(src_node);

        // Вернем TRUE
        // Сформировать node с результатом
        result_node = aml_make_node(INTEGER);
        result_node->int_value = AML_TRUE;
        aml_acquire_node(result_node);
        *out = result_node;

        printk("ACPI: CondRefOfOp: TRUE\n");
    }
    else
    {
        // Вернем FALSE
        // Сформировать node с результатом
        result_node = aml_make_node(INTEGER);
        result_node->int_value = AML_FALSE;
        aml_acquire_node(result_node);
        *out = result_node;

        printk("ACPI: CondRefOfOp: FALSE\n");
    }

    // Сохранить в target
    res = aml_store_to_target(ctx, ref_node);
    if (res != 0)
    {
        printk("ACPI: CondRefOfOp: Error writing to target %i!\n", res);
        goto exit;
    }

exit:
    aml_free_node(ref_node);
    return 0;
}

int aml_op_store(struct aml_ctx *ctx)
{
    int res;
    struct aml_node *value_node;

    res = aml_parse_next_node(ctx, &value_node);
    if (res != 0)
    {
        printk("ACPI: StoreOp: Error reading value node (%i)\n", res);
        goto exit;
    }

    // Сохранить в target
    res = aml_store_to_target(ctx, value_node);
    if (res != 0)
    {
        printk("ACPI: StoreOp: Error writing to target %i!\n", res);
        goto exit;
    }

    printk("AML Store. Value node has %i refs\n", value_node->refs.counter);

exit:
    aml_free_node(value_node); // ???
    return res;
}