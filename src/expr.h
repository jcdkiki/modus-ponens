#ifndef EXPR_H
#define EXPR_H

typedef enum {
    EXPR_IMPLIES,
    EXPR_NOT,
    EXPR_ATOM
} expr_type_t;

struct expr_t;
typedef struct expr_t {
    expr_type_t type;
    union {
        struct {
            struct expr_t *a, *b;
        } implies;
        
        struct {
            struct expr_t *a;
        } not;

        struct {
            char name[16];
        } atom;
    };
} expr_t;

int Expr_Print(expr_t *expr);
int Expr_Equal(expr_t *a, expr_t *b);

expr_t *Expr_Implies(expr_t *a, expr_t *b);
expr_t *Expr_Not(expr_t *a);
expr_t *Expr_Atom(char *name);
void    Expr_Free(expr_t* expr);
expr_t *Expr_Clone(expr_t *expr);

#endif
