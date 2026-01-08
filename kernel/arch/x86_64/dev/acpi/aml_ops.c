#include "aml.h"
#include "kairax/stdio.h"

void aml_op_alias(struct aml_ctx *ctx)
{
    printk("ALIAS");
}

void aml_op_scope(struct aml_ctx *ctx)
{
    uint32_t pkg_len = aml_read_pkg_len(ctx);
    struct aml_name_string *ns = aml_read_name_string(ctx);
    printk("SCOPE pkg_len %i\n", pkg_len);
}

void aml_op_region_op(struct aml_ctx *ctx)
{
    struct aml_name_string *region_name = aml_read_name_string(ctx);

    uint8_t region_space = aml_ctx_get_byte(ctx);
    printk("OP REGION OP name %s reg space 0x%x\n", region_name->segments->seg_s, region_space);

    struct aml_node* region_offset_node = aml_parse_next_node(ctx);
    printk("\toffset type %i value 0x%x\n", region_offset_node->type, region_offset_node->int_value);
    struct aml_node* region_len_node = aml_parse_next_node(ctx);
    printk("\tlen type %i len 0x%x\n", region_len_node->type, region_len_node->int_value);
}

// 20.2. AML Grammar Definition (998)
void aml_op_field(struct aml_ctx *ctx)
{
    uint32_t pkg_len = aml_read_pkg_len(ctx);

    struct aml_name_string *opregion_name = aml_read_name_string(ctx);
    
    uint8_t field_flags = aml_ctx_get_byte(ctx);

    printk("FIELD OP pkg_len %i name %s flags %x\n", pkg_len, opregion_name->segments->seg_s, field_flags);
}