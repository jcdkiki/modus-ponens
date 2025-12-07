#include "token.h"
#include <stdio.h>

void Token_Print(token_t *tok)
{
    switch (tok->type) {
        case TOK_IMPLIES: printf("IMPLIES"); break;
        case TOK_NOT: printf("NOT"); break;
        case TOK_ATOM: printf("ATOM %s", tok->name); break;
        case TOK_LPAREN: printf("LPAREN"); break;
        case TOK_RPAREN: printf("RPAREN"); break;
        case TOK_EOF: printf("EOF"); break;
        case TOK_ERR: printf("ERR"); break;
    }
}
