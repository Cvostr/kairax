#ifndef LEXER_H
#define LEXER_H

#include "stddef.h"

enum token_type
{
    TOKEN_STRING,
    TOKEN_CTRL_CHAR
};

struct token
{
    enum token_type type;
    union
    {
        char    punct_val;
        char*   str_val;
    };  
};

struct lexer_ctx
{
    size_t size;
    size_t offset;
    size_t saved_offset;
    const char* data;
};

int lexer_decode(const char* data, size_t len, struct token** tokens);

void lexer_save_offset(struct lexer_ctx* ctx);
void lexer_restore_offset(struct lexer_ctx* ctx);
void lexer_skip_spaces(struct lexer_ctx* ctx);
void lexer_read_string(struct lexer_ctx* ctx, int* size, char* out);

#endif