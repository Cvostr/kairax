#include "ast.h"
#include "string.h"
#include "stdlib.h"

struct ast_node* read_expression(struct ast_builder_ctx *ctx);

struct ast_node* ast_build(struct token** tokens, size_t ntokens)
{
    list* asts = create_list(); 

    struct ast_builder_ctx ctx = {
        .asts = asts,
        .tokens = tokens,
        .ntokens = ntokens,
        .offset = 0,
        .saved_offset = 0
    };   

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

int token_is_ctrl(struct token* tk) 
{
    return (tk->type == TOKEN_CTRL_CHAR);
}

int token_is_terminator(struct token* tk) 
{
    return (tk->type == TOKEN_CTRL_CHAR) && (tk->punct_val == ';' || tk->punct_val == '\n');
}

char *token_copy_str(struct token* tk) 
{
    if (tk->type != TOKEN_STRING)
        return NULL;

    char* str = malloc(strlen(tk->str_val) + 1);
    strcpy(str, tk->str_val);
    return str;
}

struct ast_node* read_expression(struct ast_builder_ctx *ctx)
{
    char ch;
    struct token* tk = ast_next_token(ctx);
    enum token_type tkn_type = tk->type;

    struct ast_node* node = malloc(sizeof(struct ast_node));
    memset(node, 0, sizeof(struct ast_node));

    struct redirect *redir_last = NULL;


    if (tkn_type == TOKEN_STRING) 
    {
        if (strcmp(tk->str_val, "if") == 0) {
            // if ast
        } else {
            // выполнение функции
            node->type = AST_NODE_FUNC;
            while (1) 
            {
                char* str = token_copy_str(tk);
                list_add(&node->func_call.args, str);

                // сразу получаем следующий токен
                tk = ast_next_token(ctx);

                // токена не оказалось - просто завершаем чтение
                if (tk == NULL) {
                    break;
                }

                // тип токена
                tkn_type = tk->type;
                // если это не строка - завершаем накопление аргументов
                if (tkn_type != TOKEN_STRING) {
                    break;
                }
            }
        }

        // Обработка того, что осталось
        if (tk != NULL)
        {
            while ((tk != NULL) && token_is_terminator(tk) == 0)
            {
                if (tkn_type == TOKEN_CTRL_CHAR)
                {
                    ch = tk->punct_val;
                    if (ch == '>') {
                        // перенаправление
                        struct redirect *redir = malloc(sizeof(struct ast_node));
                        if (node->redir_head == NULL) {
                            node->redir_head = redir;
                        } else {
                            redir_last->next = redir;
                        }
                        redir_last = redir;

                        redir->fd = 1;

                        tk = ast_next_token(ctx);
                        redir->fname = token_copy_str(tk);
                    }
                }

                tk = ast_next_token(ctx);
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