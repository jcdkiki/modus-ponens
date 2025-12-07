#include "expr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int Expr_Print(expr_t *expr)
{
    int a, b;
    switch (expr->type) {
    case EXPR_IMPLIES:
        printf("(");
        a = Expr_Print(expr->implies.a);
        printf(" => ");
        b = Expr_Print(expr->implies.b);
        printf(")");
        return 6 + a + b;

    case EXPR_NOT:
        printf("!");
        return 1 + Expr_Print(expr->not.a);

    case EXPR_ATOM:
        printf("%s", expr->atom.name);
        return strlen(expr->atom.name);
    }

    return 0;
}

int Expr_Equal(expr_t *a, expr_t *b)
{
    if (a->type != b->type) {
        return 0;
    }

    switch (a->type) {
    case EXPR_IMPLIES:
        return Expr_Equal(a->implies.a, b->implies.a) && Expr_Equal(a->implies.b, b->implies.b);
        break;

    case EXPR_NOT:
        return Expr_Equal(a->not.a, b->not.a);
        break;

    case EXPR_ATOM:
        return strcmp(a->atom.name, b->atom.name) == 0;
        break;
    }

    return 0;
}

expr_t *Expr_Clone(expr_t *expr)
{
    expr_t *clone = malloc(sizeof(expr_t));
    memcpy(clone, expr, sizeof(expr_t));

    switch (expr->type) {
    case EXPR_IMPLIES:
        clone->implies.a = Expr_Clone(expr->implies.a);
        clone->implies.b = Expr_Clone(expr->implies.b);
        break;
    case EXPR_NOT:
        clone->not.a = Expr_Clone(expr->not.a);
    default:
        break;
    }

    return clone;
}

expr_t *Expr_Implies(expr_t *a, expr_t *b)
{
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = EXPR_IMPLIES;
    expr->implies.a = a;
    expr->implies.b = b;
    return expr;
}

expr_t *Expr_Not(expr_t *a)
{
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = EXPR_NOT;
    expr->not.a = a;
    return expr;
}

expr_t *Expr_Atom(char *name)
{
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = EXPR_ATOM;
    strcpy(expr->atom.name, name);
    return expr;
}

void Expr_Free(expr_t *expr)
{
    switch (expr->type) {
    case EXPR_IMPLIES:
        Expr_Free(expr->implies.a);
        Expr_Free(expr->implies.b);
        break;
    case EXPR_NOT:
        Expr_Free(expr->not.a);
        break;
    default:
        break;
    }

    free(expr);
}
