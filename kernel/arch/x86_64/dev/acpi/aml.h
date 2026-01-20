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
#define AML_OP_STRING_PREFIX    0x0D
#define AML_OP_SCOPE        0x10
#define AML_OP_BUFFER       0x11
#define AML_OP_PACKAGE      0x12
#define AML_OP_METHOD       0x14
#define AML_OP_LOCAL0       0x60
#define AML_OP_LOCAL6       0x67
#define AML_OP_ARG0         0x68
#define AML_OP_ARG6         0x6E
#define AML_OP_ADD          0x72
#define AML_OP_SUBTRACT     0x74
#define AML_OP_MULTIPLY     0x77
#define AML_OP_DIVIDE       0x78
#define AML_OP_SHIFT_LEFT   0x79
#define AML_OP_SHIFT_RIGHT  0x7A
#define AML_OP_AND          0x7B
#define AML_OP_NAND         0x7C
#define AML_OP_OR           0x7D
#define AML_OP_NOR          0x7E
#define AML_OP_XOR          0x7F
#define AML_OP_NOOP         0x80
#define AML_OP_MOD          0x85
#define AML_OP_CREATE_DWORD_FIELD   0x8A
#define AML_OP_CREATE_WORD_FIELD    0x8B
#define AML_OP_CREATE_BYTE_FIELD    0x8C
#define AML_OP_LEQUAL       0x93
#define AML_OP_LGREATER     0x94
#define AML_OP_LLESS        0x95
#define AML_OP_IF           0xA0
#define AML_OP_RETURN       0xA4
#define AML_OP_ONES         0xFF

#define AML_EXT_OP_PREFIX       0x5B
#define AML_EXT_OP_MUTEX        0x01
#define AML_EXT_OP_REGION_OP    0x80
#define AML_EXT_OP_FIELD        0x81     
#define AML_EXT_OP_DEVICE       0x82
#define AML_EXT_OP_PROCESSOR    0x83
#define AML_EXT_OP_INDEX_FIELD  0x86

#define AML_ROOT_CHAR           '\\'
#define AML_PARENT_PREFIX_CHAR  '^'
#define AML_NAME_CHAR           '_'
#define AML_DUAL_NAME_PREFIX    '.'
#define AML_MULTI_NAME_PREFIX   '/'

struct aml_ctx {
    uint8_t *aml_data;
    uint32_t aml_len;
    uint32_t current_pos;
    struct ns_node *scope;

    struct aml_node *args[7];
};

char *aml_debug_namestring(struct aml_name_string *name);

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
int aml_read_target(struct aml_ctx *ctx, struct aml_store_target *target);
int aml_store_to_target(struct aml_store_target *target, struct aml_node *value);

struct aml_node *aml_make_node(enum aml_node_type);
void aml_free_node(struct aml_node *node);

// OP handlers
int aml_op_alias(struct aml_ctx *ctx);
int aml_op_scope(struct aml_ctx *ctx);
int aml_op_region_op(struct aml_ctx *ctx);
int aml_parse_field_list(struct aml_ctx *ctx);
int aml_op_field(struct aml_ctx *ctx);
int aml_op_index_field(struct aml_ctx *ctx);
int aml_op_method(struct aml_ctx *ctx);
int aml_op_name(struct aml_ctx *ctx);
int aml_op_device(struct aml_ctx *ctx);
int aml_op_processor(struct aml_ctx *ctx);
int aml_op_mutex(struct aml_ctx *ctx);

int aml_op_create_buffer_field(struct aml_ctx *ctx, size_t field_size);

struct aml_node *aml_op_word(struct aml_ctx *ctx);
struct aml_node *aml_op_dword(struct aml_ctx *ctx);
struct aml_node *aml_op_string(struct aml_ctx *ctx);
struct aml_node *aml_op_package(struct aml_ctx *ctx);
struct aml_node *aml_op_buffer(struct aml_ctx *ctx);

int aml_op_compare(struct aml_ctx *ctx, uint8_t opcode, struct aml_node** node_out);
int aml_op_binary(struct aml_ctx *ctx, uint8_t opcode, struct aml_node** node_out);
int aml_op_if(struct aml_ctx *ctx, struct aml_node **returned_node);

int aml_eval_string(struct aml_ctx *ctx, struct aml_node **out);
int aml_op_return(struct aml_ctx *ctx, struct aml_node **out);

int aml_to_uint64(struct aml_node* node, uint64_t *out);
int aml_node_as_integer(struct aml_node *node, uint64_t *result);

// len в байтах
int aml_read_from_field(struct aml_node *field, uint8_t *data, size_t len);
// len в байтах
int aml_write_to_field(struct aml_node *field, const uint8_t *data, size_t len);

// len в байтах
int aml_read_from_op_region(struct aml_node *region, size_t offset, size_t len, uint8_t access_sz, uint8_t *out);
int aml_read_integer_from_op_region(struct aml_node *region, size_t offset, uint8_t access_sz, uint64_t *out);
// len в байтах
int aml_write_to_op_region(struct aml_node *region, size_t offset, size_t len, uint8_t access_sz, uint8_t *in);
int aml_write_integer_to_op_region(struct aml_node *region, size_t offset, uint8_t access_sz, uint64_t in);

int aml_execute_method(struct aml_ctx *ctx, struct aml_node *method, int read_args, struct aml_node **returned_node);

#endif