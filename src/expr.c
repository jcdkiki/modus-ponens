#include "expr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_SIZE 1000000
static expr_t arena[ARENA_SIZE];
static int arena_idx = 0;

static int frees[ARENA_SIZE];
static int free_count = 0;

static int atexit_added = 0;

static int alloc_cnt = 0;
static int free_cnt = 0;

static void Expr_Check()
{
    printf("Allocated: %d exprs\n", alloc_cnt);
    printf("Freed: %d exprs\n", free_cnt);
}

static expr_t *AllocExpr()
{
    if (!atexit_added) {
        atexit_added = 1;
        atexit(Expr_Check);
    }

    if (arena_idx >= ARENA_SIZE) {
        fprintf(stderr, "Expr limit exceeded\n");
        exit(1);
    }

    alloc_cnt++;
    if (free_count > 0) {
        expr_t *expr = arena + frees[--free_count];
        memset(expr, 0, sizeof(expr_t));
        return expr;
    }

    return arena + arena_idx++;
}

void Expr_FreeItself(expr_t *expr)
{
    free_cnt++;
    int idx = expr - arena;
    frees[free_count++] = idx;
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

    Expr_FreeItself(expr);
}

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

expr_t *Expr_Implies(expr_t *a, expr_t *b)
{
    expr_t *expr = AllocExpr();
    expr->type = EXPR_IMPLIES;
    expr->implies.a = a;
    expr->implies.b = b;
    return expr;
}

expr_t *Expr_Not(expr_t *a)
{
    expr_t *expr = AllocExpr();
    expr->type = EXPR_NOT;
    expr->not.a = a;
    return expr;
}

expr_t *Expr_Atom(char *name)
{
    expr_t *expr = AllocExpr();
    expr->type = EXPR_ATOM;
    strcpy(expr->atom.name, name);
    return expr;
}

expr_t *Expr_Clone(expr_t *expr)
{
    expr_t *new_expr = AllocExpr();
    memcpy(new_expr, expr, sizeof(expr_t));

    switch (expr->type) {
    case EXPR_IMPLIES:
        new_expr->implies.a = Expr_Clone(expr->implies.a);
        new_expr->implies.b = Expr_Clone(expr->implies.b);
        break;
    case EXPR_NOT:
        new_expr->not.a = Expr_Clone(expr->not.a);
        break;
    default:
        break;
    }

    return new_expr;
}