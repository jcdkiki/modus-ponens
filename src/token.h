#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    TOK_IMPLIES,
    TOK_NOT,
    TOK_ATOM,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EOF,
    TOK_ERR
} token_type_t;

typedef struct {
    token_type_t type;
    char name[16];
} token_t;

void Token_Print(token_t *tok);

#endif
