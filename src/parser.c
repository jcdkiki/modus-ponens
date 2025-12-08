#include "parser.h"
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void Parser_ReadToken(parser_t *parser)
{
    while (isspace(parser->cur_char[0])) {
        parser->cur_char++;
    }

    token_t *tok = &parser->cur_token;

    switch (parser->cur_char[0]) {
    case '!': tok->type = TOK_NOT; parser->cur_char++; return;
    case '(': tok->type = TOK_LPAREN; parser->cur_char++; return;
    case ')': tok->type = TOK_RPAREN; parser->cur_char++; return;
    case '=':
        if (parser->cur_char[1] == '>') {
            tok->type = TOK_IMPLIES;
            parser->cur_char += 2;
            return;
        }
        else {
            tok->type = TOK_ERR;
            parser->cur_char++;
            return;
        }
        break;
    case '\0': tok->type = TOK_EOF; return;
    default:
        break;
    }

    if (!isalpha(parser->cur_char[0])) {
        tok->type = TOK_ERR;
        parser->cur_char++;
        return;
    }

    tok->type = TOK_ATOM;

    int len = 0;
    while (isalpha(parser->cur_char[0])) {
        tok->name[len] = parser->cur_char[0];
        len++;
        parser->cur_char++;
    }
    tok->name[len] = '\0';
}

void Parser_Init(parser_t *parser, const char *input)
{
    parser->cur_char = input;
    Parser_ReadToken(parser);
}

expr_t *Parser_ReadExpr(parser_t *parser)
{
    token_t *tok = &parser->cur_token;
    
    if (tok->type == TOK_NOT) {
        Parser_ReadToken(parser);
        return Expr_Not(Parser_ReadExpr(parser));
    }
    if (tok->type == TOK_ATOM) {
        char name[16];
        strcpy(name, tok->name);
        Parser_ReadToken(parser);
        return Expr_Atom(name);
    }
    if (tok->type == TOK_LPAREN) {
        Parser_ReadToken(parser);
        expr_t *left = Parser_ReadExpr(parser);
        if (parser->cur_token.type != TOK_IMPLIES) {
            printf("Expected '=>', got ");
            Token_Print(tok);
            exit(1);
        }
        Parser_ReadToken(parser);
        expr_t *right = Parser_ReadExpr(parser);
        if (parser->cur_token.type != TOK_RPAREN) {
            printf("Expected ')', got ");
            Token_Print(tok);
            exit(1);
        }
        Parser_ReadToken(parser);
        return Expr_Implies(left, right);
    }

    printf("Expected '!', '(', or atom, got ");
    Token_Print(tok);
    exit(1);
}
