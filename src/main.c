#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N_AXIOMS 3

#define MAX_EXPRS 1000
expr_t *pool[MAX_EXPRS];
int pool_count = 0;

void AddToPool(expr_t* e) {
    if (pool_count < MAX_EXPRS) {
        pool[pool_count++] = e;
    }
    else {
        printf("POOL IS FULL!!!\n");
    }
}

expr_t* Substitute(expr_t* template, expr_t* subA, expr_t* subB, expr_t* subC)
{
    if (template->type == EXPR_ATOM) {
        if (strcmp(template->atom.name, "A") == 0 && subA != NULL) return Expr_Clone(subA);
        if (strcmp(template->atom.name, "B") == 0 && subB != NULL) return Expr_Clone(subB);
        if (strcmp(template->atom.name, "C") == 0 && subC != NULL) return Expr_Clone(subC);
        return Expr_Clone(template);
    } 
    else if (template->type == EXPR_IMPLIES) {
        return Expr_Implies(
            Substitute(template->implies.a, subA, subB, subC),
            Substitute(template->implies.b, subA, subB, subC)
        );
    }
    return NULL;
}

expr_t *TryModusPonens(expr_t *a_impl_b, expr_t *a)
{
    if (a_impl_b->type == EXPR_IMPLIES) {
        if (Expr_Equal(a_impl_b->implies.a, a)) {
            return Expr_Clone(a_impl_b->implies.b);
        }
    }
    return NULL;
}

void RunInference() {
    int new_found = 1;
    while (new_found) {
        new_found = 0;
        int current_cnt = pool_count;
        
        for (int i = 0; i < current_cnt; i++) {
            for (int j = 0; j < current_cnt; j++) {
                
                expr_t *res = TryModusPonens(pool[i], pool[j]);
                
                if (res) {
                    int exists = 0;
                    for (int k = 0; k < pool_count; k++) {
                        if(Expr_Equal(pool[k], res)) {
                            exists = 1;
                            break;
                        }
                    }

                    if (!exists) {
                        printf("MP: ");
                        Expr_Print(pool[i]);
                        printf(",\n     ");
                        int len = Expr_Print(pool[j]);
                        putc('\n', stdout);
                        while (len--) putc(' ', stdout);
                        printf("      |- ");
                        Expr_Print(res);
                        printf("\n\n");

                        AddToPool(res);
                        new_found = 1;
                    } else {
                        Expr_Free(res);
                    }
                }
            }
        }
    }
}

int main() {
    expr_t *A = Expr_Atom("A"); 
    expr_t *B = Expr_Atom("B");
    expr_t *C = Expr_Atom("C");
    
    expr_t *Ax1 = Expr_Implies(A, Expr_Implies(B, A));
    
    expr_t *Ax2 = Expr_Implies(
        Expr_Implies(A, Expr_Implies(B, C)), 
        Expr_Implies(
            Expr_Implies(A, B),
            Expr_Implies(A, C)
        )
    );

    printf("A1: "); Expr_Print(Ax1); printf("\n");
    printf("A2: "); Expr_Print(Ax2); printf("\n");
    
    expr_t *p = Expr_Atom("p");

    expr_t *step1 = Substitute(Ax1, p, Expr_Implies(p, p), NULL);
    AddToPool(step1);
    
    expr_t *step2 = Substitute(Ax2, p, Expr_Implies(p, p), p);
    AddToPool(step2);
    
    expr_t *step3 = Substitute(Ax1, p, p, NULL);
    AddToPool(step3);

    RunInference();

    return 0;
}
