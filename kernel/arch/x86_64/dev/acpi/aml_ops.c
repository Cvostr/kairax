#include "aml.h"
#include "kairax/stdio.h"

void aml_op_alias(struct aml_ctx *ctx)
{
    printk("ALIAS");
}

int aml_op_scope(struct aml_ctx *ctx)
{
    int rc;
    size_t len;
    uint8_t *nested = aml_ctx_dup_from_pkg(ctx, &len);

    struct aml_ctx parse_ctx;
    parse_ctx.aml_data = nested;
    parse_ctx.aml_len = len;
    parse_ctx.current_pos = 0;

    struct aml_name_string *ns = aml_read_name_string(&parse_ctx);
    printk("SCOPE pkg_len %i, name segments %i\n", len, ns->segments_num);

    struct aml_node *node;

    while (parse_ctx.current_pos < parse_ctx.aml_len)
    {
        rc = aml_parse_next_node(&parse_ctx, &node);
        if (rc != 0)
        {
            kfree(nested);
            return rc;
        }
    }

    kfree(nested);

    return 0;
}

void aml_op_region_op(struct aml_ctx *ctx)
{
    struct aml_name_string *region_name = aml_read_name_string(ctx);

    uint8_t region_space = aml_ctx_get_byte(ctx);
    //printk("OP REGION OP name %s reg space 0x%x\n", region_name->segments->seg_s, region_space);

    struct aml_node *region_offset_node, *region_len_node;
    
    aml_parse_next_node(ctx, &region_offset_node);
    //printk("\toffset type %i value 0x%x\n", region_offset_node->type, region_offset_node->int_value);
    
    aml_parse_next_node(ctx, &region_len_node);
    //printk("\tlen type %i len 0x%x\n", region_len_node->type, region_len_node->int_value);
}

void aml_parse_field_list(struct aml_ctx *ctx)
{
    uint8_t cur; 
    
    while (ctx->current_pos < ctx->aml_len)
    {
        cur = aml_ctx_peek_byte(ctx);
        switch (cur)
        {
            case 0:
                aml_ctx_get_byte(ctx);
                uint8_t access_type = aml_ctx_get_byte(ctx);
                uint8_t access_attrib = aml_ctx_get_byte(ctx);
                printk("\treserved field access_type %i access_attrib %i\n", access_type, access_attrib);
                break;
            case 1:
                printk("\taccess field\n");
                break;
            case 2:
                printk("\tconnect field\n");
                break;
            case 3:
                printk("\textended access field\n");
                break;
            default:
                char seg[5];
                seg[4] = 0;
                aml_ctx_copy_bytes(ctx, seg, 4);
                uint32_t len = aml_read_pkg_len(ctx);
                printk("\tnamed field %s len %i\n", seg, len);
                break;
        }
    }
}

// 20.2. AML Grammar Definition (998)
void aml_op_field(struct aml_ctx *ctx)
{
    // Сделаем копию пакета
    size_t len;
    uint8_t *nested = aml_ctx_dup_from_pkg(ctx, &len);
    // Изза особенностей функций мы зависим от aml_ctx. 
    // Сделаем еще один фиктивный, ну а хули нет?
    struct aml_ctx field_ctx;
    field_ctx.aml_data = nested;
    field_ctx.aml_len = len;
    field_ctx.current_pos = 0;

    struct aml_name_string *opregion_name = aml_read_name_string(&field_ctx);
    uint8_t field_flags = aml_ctx_get_byte(&field_ctx);

    printk("FIELD OP pkg_len %i name '%s' flags %x\n", len, opregion_name->segments->seg_s, field_flags);

    aml_parse_field_list(&field_ctx);

    kfree(nested);
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

    struct aml_name_string *method_name = aml_read_name_string(&method_ctx);

    uint8_t method_flags = aml_ctx_get_byte(&method_ctx);
    uint8_t arg_count = method_flags & 0b111;
    uint8_t serialize = (method_flags > 3) & 1;
    uint8_t sync_level = (method_flags > 4) & 0xF;

    printk("METHOD OP pkg_len %i name '%s' args %i, serialized %i, sync level %i\n", 
        len, method_name->segments->seg_s, arg_count, serialize, sync_level);

    kfree(nested);
}

void aml_op_name(struct aml_ctx *ctx)
{
    struct aml_name_string *name = aml_read_name_string(ctx);
    printk("OPNAME '%s'\n", name->segments->seg_s);

    struct aml_node *value;
    aml_parse_next_node(ctx, &value);

    // TODO: make node?
    // может просто добавлять ноду value с именем name
}

struct aml_node *aml_op_package(struct aml_ctx *ctx)
{
    int rc;
    size_t len;
    uint8_t *pkg_buf = aml_ctx_dup_from_pkg(ctx, &len);

    struct aml_ctx pkg_ctx;
    pkg_ctx.aml_data = pkg_buf;
    pkg_ctx.aml_len = len;
    pkg_ctx.current_pos = 0;

    uint8_t num_elements = aml_ctx_get_byte(&pkg_ctx);

    printk("OP PACKAGE len %i num_elements %i\n", len, num_elements);

    struct aml_node *node = aml_make_node(PACKAGE);
    node->package.num_elements = num_elements;

    struct aml_node *value;

    for (int i = 0; i < num_elements; i ++)
    {
        rc = aml_parse_next_node(&pkg_ctx, &value);
        if (rc != 0)
        {
            printk("ACPI: Error (%i) handling package element\n", rc);
            return NULL;
        }

        // TODO: сохранить элемент
    }

    kfree(pkg_buf);

    return node;
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