#include "aml.h"
#include "mem/kheap.h"
#include "kairax/string.h"
#include "kairax/ctype.h"
#include "kairax/stdio.h"

size_t aml_ctx_get_remain_size(struct aml_ctx *ctx)
{
    return ctx->aml_len - ctx->current_pos;
}

uint8_t aml_ctx_get_byte(struct aml_ctx *ctx)
{
    return ctx->aml_data[ctx->current_pos++];
}

uint8_t aml_ctx_next_byte(struct aml_ctx *ctx)
{
    return ctx->aml_data[++ctx->current_pos];
}

uint8_t aml_ctx_peek_byte(struct aml_ctx *ctx)
{
    return ctx->aml_data[ctx->current_pos];
}

uint8_t *aml_ctx_copy_bytes(struct aml_ctx *ctx, size_t len)
{
    if (aml_ctx_get_remain_size(ctx) < len)
        return NULL;

    uint8_t *res = kmalloc(len);
    memcpy(res, ctx->aml_data + ctx->current_pos, len);
    ctx->current_pos += len;
    return res;
}

struct aml_node *aml_make_node(enum aml_node_type type)
{
    struct aml_node *node = kmalloc(sizeof(struct aml_node));
    node->type = type;
    return node;
}

int aml_is_namestring_char_valid(uint8_t c, int islead)
{
    if (islead)
        return (c >= 'A' && c <= 'Z') || c == '_';
    
    return (c >= 'A' && c <= 'Z') || c == '_' || isdigit(c);
}

// Advanced Configuration and Power Interface (ACPI) Specification, Version 6.4
// 20.2.4 Package Length Encoding
uint32_t aml_read_pkg_len(struct aml_ctx *ctx)
{
    uint8_t lead_byte = aml_ctx_get_byte(ctx);
    uint8_t enc_len = lead_byte >> 6;
    uint32_t tmp;
    uint32_t pkg_len = 0;

    switch (enc_len)
    {
        case 0:
            pkg_len = lead_byte & 0b111111;
            break;
        case 1:
            pkg_len = lead_byte & 0xF;
            tmp = aml_ctx_get_byte(ctx);
            pkg_len |= tmp << 4;
            break;
        case 2:
            pkg_len = lead_byte & 0xF;
            tmp = aml_ctx_get_byte(ctx);
            pkg_len |= tmp << 4;
            tmp = aml_ctx_get_byte(ctx);
            pkg_len |= tmp << 12;
            break;
        case 3:
            pkg_len = lead_byte & 0xF;
            tmp = aml_ctx_get_byte(ctx);
            pkg_len |= tmp << 4;
            tmp = aml_ctx_get_byte(ctx);
            pkg_len |= tmp << 12;
            tmp = aml_ctx_get_byte(ctx);
            pkg_len |= tmp << 20;
            break;
    }

    return pkg_len;
}

struct aml_name_string *aml_read_name_string(struct aml_ctx *ctx)
{
    int string_base = 0;
    uint8_t chr = aml_ctx_peek_byte(ctx);
    //printk("ACPI STR. CHR 1 %c\n", chr);
    
    switch (chr)
    {
    case '^':
        while (chr == '^') {
			string_base++;
			chr = aml_ctx_next_byte(ctx);
		}
        break;
    case '\\':
        chr = aml_ctx_next_byte(ctx);
        string_base = -1; // TODO: figure out
        break;
    }

    //printk("ACPI STR. CHR 2 %c %i\n", chr, chr);

    size_t segments = 1;
    switch (chr)
    {
    case 0:
        segments = 0;
        chr = aml_ctx_next_byte(ctx);
        break;
    case '.':
        segments = 2;
        chr = aml_ctx_next_byte(ctx);
        break;
    case '/':
        segments = aml_ctx_next_byte(ctx);
        chr = aml_ctx_get_byte(ctx);
        break;
    }
    //printk("ACPI STR. CHR %c %i SEGMENTS %i\n", chr, chr, segments);

    // выделяем память - размер заголовка + сегменты по 4 байта
    struct aml_name_string *res = kmalloc(sizeof(struct aml_name_string) + segments * 4);
    res->base = string_base;

    for (int i = 0; i < segments; i ++)
    {
        for (int j = 0; j < 4; j ++)
        {
            uint8_t v = aml_ctx_get_byte(ctx);
            if (aml_is_namestring_char_valid(v, j == 0) == FALSE)
            {
                printk("ACPI: NameString segment char is not valid: %c\n", v);
                return -EINVAL;
            }
            res->segments[i].seg_s[j] = v;
            //printk("%c", v);
        }
    }

    return res;
}

int aml_parse(char *data, uint32_t len)
{
    int rc;
    struct aml_ctx parse_ctx;
    parse_ctx.aml_data = data;
    parse_ctx.aml_len = len;
    parse_ctx.current_pos = 0;

    while (parse_ctx.current_pos < parse_ctx.aml_len)
    {
        rc = aml_parse_next_node(&parse_ctx);
        if (rc != 0)
            return rc;
    }

    return 0;
}

void aml_op_alias(struct aml_ctx *ctx);

struct aml_node* aml_parse_next_node(struct aml_ctx *ctx)
{
    struct aml_node *node = NULL;
    uint8_t opcode = aml_ctx_get_byte(ctx);

    if (opcode == AML_EXT_OP_PREFIX)
    {
        opcode = aml_ctx_get_byte(ctx);
        switch (opcode)
        {
            case AML_EXT_OP_REGION_OP:
                aml_op_region_op(ctx);
                break;
            case AML_EXT_OP_FIELD:
                aml_op_field(ctx);
                break;
            
            default:
                printk("ACPI: Unknown ext opcode %x\n", opcode);
                return -EINVAL;
        }
    }
    else
    {
        switch (opcode)
        {
        case AML_OP_ONE:
            node = aml_make_node(INTEGER);
            node->int_value = 1;
            break;
        case AML_OP_ALIAS:
            aml_op_alias(ctx);
            break;
        case AML_OP_NAME:
            break;
        case AML_OP_SCOPE:
            aml_op_scope(ctx);
            break;
        case AML_OP_BYTE_PREFIX:
            node = aml_make_node(INTEGER);
            node->int_value = aml_ctx_get_byte(ctx);
            break;
        case AML_OP_WORD_PREFIX:
            node = aml_make_node(INTEGER);
            node->int_value = aml_ctx_get_byte(ctx);
            node->int_value <<= 8;
            node->int_value |= aml_ctx_get_byte(ctx);
            break;
        case AML_OP_NOOP:
            //printk("ACPI: NOOP\n");
            break;
        
        default:
            printk("ACPI: Unknown opcode %x\n", opcode);
            return -EINVAL;
        }
    }

    return node;
}