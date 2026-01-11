#ifndef ACPI_AML_H
#define ACPI_AML_H

#include "aml_types.h"
#include "acpi_namespace.h"

#define AML_OP_ZERO         0x00
#define AML_OP_ONE          0x01
#define AML_OP_ALIAS        0x06
#define AML_OP_NAME         0x08
#define AML_OP_BYTE_PREFIX  0x0A
#define AML_OP_WORD_PREFIX  0x0B
#define AML_OP_DWORD_PREFIX 0x0C
#define AML_OP_SCOPE        0x10
#define AML_OP_BUFFER       0x11
#define AML_OP_PACKAGE      0x12
#define AML_OP_METHOD       0x14
#define AML_OP_NOOP         0x80
#define AML_OP_ONES         0xFF

#define AML_EXT_OP_PREFIX       0x5B
#define AML_EXT_OP_REGION_OP    0x80
#define AML_EXT_OP_FIELD        0x81     

#define AML_EXT_OP_INDEX_FIELD  0x86

struct aml_ctx {
    uint8_t *aml_data;
    uint32_t aml_len;
    uint32_t current_pos;
    struct ns_node *scope;
};

int aml_parse(char *data, uint32_t len);
int aml_parse_next_node(struct aml_ctx *ctx, struct aml_node** node_out);

size_t aml_ctx_get_remain_size(struct aml_ctx *ctx);
uint8_t aml_ctx_get_byte(struct aml_ctx *ctx);
uint8_t aml_ctx_next_byte(struct aml_ctx *ctx);
uint8_t aml_ctx_peek_byte(struct aml_ctx *ctx);
int aml_ctx_copy_bytes(struct aml_ctx *ctx, uint8_t *out, size_t len);
uint32_t aml_read_pkg_len(struct aml_ctx *ctx);
uint8_t *aml_ctx_dup_from_pkg(struct aml_ctx *ctx, size_t *len);
uint32_t aml_ctx_addr_from_pkg(struct aml_ctx *ctx, uint8_t **begin_addr);
struct aml_name_string *aml_read_name_string(struct aml_ctx *ctx);

struct aml_node *aml_make_node(enum aml_node_type);

// OP handlers
void aml_op_alias(struct aml_ctx *ctx);
int aml_op_scope(struct aml_ctx *ctx);
int aml_op_region_op(struct aml_ctx *ctx);
int aml_parse_field_list(struct aml_ctx *ctx);
int aml_op_field(struct aml_ctx *ctx);
void aml_op_index_field(struct aml_ctx *ctx);
void aml_op_method(struct aml_ctx *ctx);
int aml_op_name(struct aml_ctx *ctx);
struct aml_node *aml_op_word(struct aml_ctx *ctx);
struct aml_node *aml_op_dword(struct aml_ctx *ctx);
struct aml_node *aml_op_package(struct aml_ctx *ctx);
struct aml_node *aml_op_buffer(struct aml_ctx *ctx);

#endif