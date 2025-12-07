#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "expr.h"

typedef struct {
    token_t cur_token;
    const char *cur_char;
} parser_t;

void Parser_Init(parser_t *parser, const char *input);
expr_t *Parser_ReadExpr(parser_t *parser);

#endif
