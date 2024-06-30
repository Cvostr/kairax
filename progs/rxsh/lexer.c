#include "lexer.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"

char ctrl_chars[] = {'|', '<', '>', '&', ';', '!', '$', '(', ')', '\n'};
int ctrl_chars_size = sizeof(ctrl_chars) / sizeof(char);

int lexer_is_ctrl_char(char c);

int lexer_decode(const char* data, size_t len, struct token** tokens)
{
    int ntokens = 0;
    struct token* token = NULL;
    struct lexer_ctx ctx = 
    {
        .data = data,
        .size = len,
        .offset = 0
    };

    while (ctx.offset < ctx.size)
    {
        token = NULL;
        lexer_skip_spaces(&ctx);

        if ((ctx.offset + 1) >= ctx.size)
        {
            break;
        }

        char c = ctx.data[ctx.offset];
        int len = 0;

        if (lexer_is_ctrl_char(c)) {
            token = malloc(sizeof(struct token)); 
            token->type = TOKEN_CTRL_CHAR;
            token->punct_val = c;
            ctx.offset++;
        } else {
            lexer_save_offset(&ctx);
            lexer_read_string(&ctx, &len, NULL);
            token = malloc(sizeof(struct token)); 
            token->type = TOKEN_STRING;
            token->str_val = malloc(len + 1);
            memset(token->str_val, 0, len + 1);
            lexer_restore_offset(&ctx);
            lexer_read_string(&ctx, NULL, token->str_val);
        }

        tokens[ntokens++] = token;
    }

    return ntokens;
}

int lexer_is_ctrl_char(char c) 
{
    for (int i = 0; i < ctrl_chars_size; i ++)
    {
        if (c == ctrl_chars[i]) {
            return 1;
        }
    }

    return 0;
}

int lexer_is_string_terminator(char c)
{
    return lexer_is_ctrl_char(c) || isspace(c);
}

void lexer_skip_spaces(struct lexer_ctx* ctx)
{
    while (ctx->offset < ctx->size && isspace(ctx->data[ctx->offset]))
    {
        ctx->offset ++;
    }
}

void lexer_save_offset(struct lexer_ctx* ctx)
{
    ctx->saved_offset = ctx->offset;
}

void lexer_restore_offset(struct lexer_ctx* ctx)
{
    ctx->offset = ctx->saved_offset;
}

void lexer_read_string(struct lexer_ctx* ctx, int* size, char* out)
{
    int len = 0;
    char c = ctx->data[ctx->offset];
    char endc = 0;
    if (c == '"' || c == '\'') 
    {
        endc = c;
        ctx->offset++;
        while (ctx->offset < ctx->size && ctx->data[ctx->offset] != endc)
        {      
            c = ctx->data[ctx->offset++];
            if (out) {
                out[len] = c;
            }
            len++;
        }
    } else {
        while (ctx->offset < ctx->size && !lexer_is_string_terminator(ctx->data[ctx->offset]))
        {      
            c = ctx->data[ctx->offset++];

            if (out) {
                out[len] = c;
            }
            len++;
        }
    }

    if (size) {
        *size = len;
    }
}