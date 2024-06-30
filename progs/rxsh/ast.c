#include "ast.h"
#include "string.h"
#include "stdlib.h"

struct ast_node* read_expression(struct ast_builder_ctx *ctx);

struct ast_node* ast_build(struct token** tokens, size_t ntokens)
{
    struct ast_builder_ctx ctx = {
        .tokens = tokens,
        .ntokens = ntokens,
        .offset = 0,
        .saved_offset = 0
    };   

    list* asts = create_list(); 

    while (ctx.offset < ctx.ntokens)
    {
        struct ast_node* expr_node = read_expression(&ctx);
        list_add(asts, expr_node);
    }

    struct ast_node* node = malloc(sizeof(struct ast_node));
    memset(node, 0, sizeof(struct ast_node));
    node->type = AST_NODE_ROOT;
    node->nodes = asts;

    return node;
}

int token_is_terminator(struct token* tk) 
{
    return (tk->type == TOKEN_CTRL_CHAR && tk->punct_val == ';');
}

int token_is_ctrl(struct token* tk) 
{
    return (tk->type == TOKEN_CTRL_CHAR);
}

struct ast_node* read_expression(struct ast_builder_ctx *ctx)
{
    struct token* tk = ast_next_token(ctx);

    struct ast_node* node = malloc(sizeof(struct ast_node));
    memset(node, 0, sizeof(struct ast_node));

    if (tk->type == TOKEN_STRING) 
    {
        if (strcmp(tk->str_val, "if") == 0) {
            // if ast
        } else {
            // выполнение функции
            node->type = AST_NODE_FUNC;
            while (1) 
            {
                char* str = malloc(strlen(tk->str_val) + 1);
                strcpy(str, tk->str_val);
                list_add(&node->func_call.args, str);

                tk = ast_next_token(ctx);
                if (tk == NULL) {
                    break;
                }
                if (token_is_ctrl(tk)) {
                    break;
                }
            }
        }
    }

    return node;
}

struct token* ast_next_token(struct ast_builder_ctx* ctx)
{
    if (ctx->offset >= ctx->ntokens) {
        return NULL;
    }

    return ctx->tokens[ctx->offset++];
}