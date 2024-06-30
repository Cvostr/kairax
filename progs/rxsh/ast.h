#ifndef AST_H
#define AST_H

#include "stddef.h"
#include "lexer.h"
#include "list.h"

enum ast_node_type
{
    AST_NODE_ROOT,
    AST_NODE_IF,
    AST_NODE_WHILE,
    AST_NODE_FUNC,
    AST_NODE_PIPE
};

struct ast_node
{
    enum ast_node_type type;

    union 
    {
        list* nodes;

        struct {
            list args;
        } func_call;  

        struct {
            struct ast_node* sender;
            struct ast_node* receiver;
        } pipe;
    };


    void (*free) (struct ast_node*);
};

struct ast_builder_ctx
{
    struct token** tokens;
    size_t ntokens;
    size_t offset;
    size_t saved_offset;
};

struct ast_node* ast_build(struct token** tokens, size_t ntokens);

struct token* ast_next_token(struct ast_builder_ctx* ctx);

void ast_execute(struct ast_node* node);

#endif