#include "parser.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_EXPRS 100000
#define MAX_TERMS 1000

typedef enum {
    INFERENCE_AXIOM,
    INFERENCE_MODUS_PONENS,
    INFERENCE_DEDUCTION
} inference_type_t;

typedef struct true_expr_t {
    expr_t *e;
    inference_type_t type;
    int idx;

    union {
        struct {
            expr_t *axiom;
            expr_t *A, *B, *C;
        } axiom;
        
        struct {
            struct expr_t *A_impl_B;
            struct expr_t *A;
        } modus_ponens;

        struct {
            // todo;
        } deduction;
    };
    
    int printed;
    bool visited;
} true_expr_t;

uint64_t TrueExpr_Hash(true_expr_t te)
{
    return Expr_Hash(te.e);
}

bool TrueExpr_Cmp(true_expr_t a, true_expr_t b)
{
    return Expr_Equal(a.e, b.e);
}

#define NAME pool_map
#define KEY_TY expr_t*
#define VAL_TY true_expr_t
#define HASH_FN Expr_Hash
#define CMPR_FN Expr_Equal
#include "verstable.h"

#define NAME terms_set
#define KEY_TY expr_t*
#define HASH_FN Expr_Hash
#define CMPR_FN Expr_Equal
#include "verstable.h"

pool_map pool;
terms_set terms;

static int print_axioms = 0;
static int print_history = 1;
static int add_neg_terms = 0;
static int add_self_impl = 0;

#define NOT_FOUND -1

true_expr_t *FindExprInPool(expr_t *e)
{
    pool_map_itr iter = pool_map_get(&pool, e);
    if (pool_map_is_end(iter)) {
        return NULL;
    }
    return &iter.data->val;
}

expr_t *FindExprInTerms(expr_t *e)
{
    terms_set_itr iter = terms_set_get(&terms, e);
    if (terms_set_is_end(iter)) {
        return NULL;
    }
    return iter.data->key;
}

void TrueExpr_Init(true_expr_t *te, expr_t *e)
{
    te->e = e;
    te->printed = 0;
    te->idx = pool_map_size(&pool);
    te->visited = false;
}

void AddToPool(true_expr_t* te)
{
    pool_map_itr iter = pool_map_insert(&pool, te->e, *te);
}

void TrueExpr_ModusPonens(true_expr_t *te, expr_t *A_impl_B, expr_t *A)
{
    te->type = INFERENCE_MODUS_PONENS;
    te->modus_ponens.A = A;
    te->modus_ponens.A_impl_B = A_impl_B;
}

void TrueExpr_Axiom(true_expr_t *te, expr_t *axiom, expr_t *A, expr_t *B, expr_t *C)
{
    te->type = INFERENCE_AXIOM;
    te->axiom.axiom = axiom;
    te->axiom.A = A;
    te->axiom.B = B;
    te->axiom.C = C;
}

void TrueExpr_Deduction(true_expr_t *te)
{
    te->type = INFERENCE_DEDUCTION;
}

void AddTerm(expr_t* e)
{
    terms_set_insert(&terms, e);
}

void ExtractSubformulas(expr_t* e, int negate)
{
    switch (e->type) {
    case EXPR_ATOM:
        break;
    case EXPR_IMPLIES:
        ExtractSubformulas(e->implies.a, negate);
        ExtractSubformulas(e->implies.b, negate);
        break;
    case EXPR_NOT:
        ExtractSubformulas(e->not.a, !negate);
        break;
    default:
        ASSERT(0, "Unknown expression type");
        break;
    }
    
    if (negate) AddTerm(Expr_Not(e));
    else AddTerm(e);
}

expr_t* Substitute(expr_t* template, expr_t* subA, expr_t* subB, expr_t* subC)
{
    switch (template->type) {
    case EXPR_ATOM:
        if (strcmp(template->atom.name, "A") == 0 && subA != NULL) return Expr_Clone(subA);
        if (strcmp(template->atom.name, "B") == 0 && subB != NULL) return Expr_Clone(subB);
        if (strcmp(template->atom.name, "C") == 0 && subC != NULL) return Expr_Clone(subC);
        ASSERT(0, "Every 'variable' should be replaced");
        break;

    case EXPR_IMPLIES:
        return Expr_Implies(
            Substitute(template->implies.a, subA, subB, subC),
            Substitute(template->implies.b, subA, subB, subC)
        );
    
    case EXPR_NOT:
        return Expr_Not(
            Substitute(template->not.a, subA, subB, subC)
        );
    
    default:
        ASSERT(0, "Unknown expression type");
        break;
    }

    return NULL;
}

void PrintAxiom(true_expr_t *te)
{
    printf("   Axiom = ");
    Expr_Print(te->axiom.axiom);
    printf("\n       A = ");
    Expr_Print(te->axiom.A);
    printf("\n       B = ");
    Expr_Print(te->axiom.B);
    printf("\n       C = ");
    Expr_Print(te->axiom.C);
    printf("\n");

    printf("  %3d ", te->idx);
    Expr_Print(te->e);
    printf("\n\n");
}

void PrintModusPonens(true_expr_t *te)
{
    true_expr_t *A_impl_B = FindExprInPool(te->modus_ponens.A_impl_B);
    true_expr_t *A = FindExprInPool(te->modus_ponens.A);

    printf("%5d ", A_impl_B->idx);
    Expr_Print(A_impl_B->e);
    printf(",\n%5d  ", A->idx); 
    int len = Expr_Print(A->e);
    printf("\n%5d ", te->idx);
    while (len--) putc(' ', stdout);
    printf("  |- ");
    Expr_Print(te->e);
    printf("\n\n");
}

void InstantiateAxiom(expr_t* ax)
{
    if (print_axioms) {
        printf("Axiom: ");
        Expr_Print(ax);
        printf("\n");
    }

    terms_set_itr begin = terms_set_first(&terms);
    for (terms_set_itr it_i = begin; !terms_set_is_end(it_i); it_i = terms_set_next(it_i)) {
        for (terms_set_itr it_j = begin; !terms_set_is_end(it_j); it_j = terms_set_next(it_j)) {
            for (terms_set_itr it_k = begin; !terms_set_is_end(it_k); it_k = terms_set_next(it_k)) {
                expr_t *A = it_i.data->key;
                expr_t *B = it_j.data->key;
                expr_t *C = it_k.data->key;

                expr_t *instance = Substitute(ax, A, B, C);
                
                if (FindExprInTerms(instance) == NULL) {
                    true_expr_t te;
                    TrueExpr_Init(&te, instance);
                    TrueExpr_Axiom(&te, ax, A, B, C);
                    AddToPool(&te);
                    if (print_axioms) PrintAxiom(&te);
                }
                else {
                    Expr_Free(instance);
                }
            }
        }
    }
}

terms_set assumptions;

bool ProveWithAssumptions(expr_t* goal, terms_set* temp_assumptions, int depth);

bool TryDeduction(expr_t* A)
{
    terms_set local_assumptions;
    terms_set_init(&local_assumptions);

    bool result = ProveWithAssumptions(A, &local_assumptions, 0);
    
    terms_set_clear(&local_assumptions);
    return result;
}

#define DEPTH for (int i = 0; i < depth; i++) printf("  ");

bool ProveWithAssumptions(expr_t* goal, terms_set* temp_assumptions, int depth)
{
    if (depth > 50) {
        return false;
    }

    //DEPTH printf("Prove "); Expr_Print(goal); printf("\n");
    
    if (FindExprInPool(goal) != NULL) {
        //DEPTH printf("OK! (Already proven)\n");
        return true;
    }

    terms_set_itr it_assume = terms_set_get(temp_assumptions, goal);
    if (!terms_set_is_end(it_assume)) {
        //DEPTH printf("OK! (assumed)\n");
        return true;
    }

    for (pool_map_itr it = pool_map_first(&pool); !pool_map_is_end(it); it = pool_map_next(it)) {
        true_expr_t *te = &it.data->val;
        expr_t *e = te->e;

        if (te->visited) continue;

        if (e->type == EXPR_IMPLIES && Expr_Equal(e->implies.b, goal)) {
            te->visited = true;
            bool ok = ProveWithAssumptions(e->implies.a, temp_assumptions, depth+1);
            te->visited = false;
            if (ok) {
                //DEPTH printf("OK! (found X => goal, proved X)\n");
                return true;    
            }
        }
    }

    // A => B
    if (goal->type == EXPR_IMPLIES) {
        expr_t* A = goal->implies.a;
        expr_t* B = goal->implies.b;
        
        bool ok = false;
        if (terms_set_is_end(terms_set_get(temp_assumptions, A))) {
            //DEPTH printf("Assume "); Expr_Print(A); printf("\n");
            terms_set_insert(temp_assumptions, A);
            ok = ProveWithAssumptions(B, temp_assumptions, depth+1);
            terms_set_erase(temp_assumptions, A);
        }
        else ok = ProveWithAssumptions(B, temp_assumptions, depth+1);

        if (ok) {
            //DEPTH printf("OK! (assume A, prove B)\n");
            return true;
        }
    }

    return false;
}

true_expr_t *RunInference(expr_t* goal)
{
    int cycle = 0;
    int new_found = 1;
    int goal_found = 0;

    while (new_found != 0 && !goal_found && cycle < 2) {
        new_found = 0;
        cycle++;
        
        for (pool_map_itr it = pool_map_first(&pool); !pool_map_is_end(it) && !goal_found;) {
            expr_t *A_impl_B = it.data->val.e;
            
            if (A_impl_B->type != EXPR_IMPLIES) {
                continue;
            }
            expr_t *A = A_impl_B->implies.a;
            expr_t *B = A_impl_B->implies.b;
            
            if (FindExprInPool(B) != NULL) {
                it = pool_map_next(it);
                continue;
            }

            true_expr_t *A_te = FindExprInPool(A);
            //if (A_te == NULL) {
            //    bool res = TryDeduction(A);
            //    if (res) {
            //        printf("DEDUCTED "); Expr_Print(A); printf("\n");
            //        true_expr_t te;
            //        TrueExpr_Init(&te, B);
            //        TrueExpr_Deduction(&te);
            //        AddToPool(&te); // iterator gets invalidated.... :(((
            //        it = pool_map_first(&pool);
            //        new_found = 1;
            //        continue;
            //    }
            //}
            
            if (A_te != NULL) {
                true_expr_t te;
                TrueExpr_Init(&te, B);
                TrueExpr_ModusPonens(&te, A_impl_B, A);
                AddToPool(&te); // iterator gets invalidated.... :(((
                it = pool_map_first(&pool);
                new_found = 1;
                
                if (Expr_Equal(B, goal)) {
                    printf("GOAL FOUND!\n");
                    goal_found = 1;
                }
                continue;
            }

            it = pool_map_next(it);
        }
    }

    return FindExprInPool(goal); 
}

void PrintHistory(true_expr_t *te)
{
    if (te->printed) return;
    te->printed = 1;

    switch (te->type) {
    case INFERENCE_AXIOM:
        PrintAxiom(te);
        break;
    case INFERENCE_MODUS_PONENS:
        PrintHistory(FindExprInPool(te->modus_ponens.A));
        PrintHistory(FindExprInPool(te->modus_ponens.A_impl_B));
        PrintModusPonens(te);
        break;
    case INFERENCE_DEDUCTION:
        printf("%5d DEDUCTED ", te->idx);
        Expr_Print(te->e);
        printf("\n");
        break;
    }
}

int main(int argc, char **argv) {
    argv++;

    if (*argv == NULL) {
        printf("specify axioms file\n");
        return 1;
    }

    pool_map_init(&pool);
    terms_set_init(&terms);
    terms_set_init(&assumptions);

    char *filename = *argv;
    argv++;
    
    while (*argv != NULL) {
        if (strcmp(*argv, "+axioms") == 0) print_axioms = 1;
        else if (strcmp(*argv, "-axioms") == 0) print_axioms = 0;
        else if (strcmp(*argv, "+history") == 0) print_history = 1;
        else if (strcmp(*argv, "-history") == 0) print_history = 0;
        else if (strcmp(*argv, "+neg") == 0) add_neg_terms = 1;
        else if (strcmp(*argv, "-neg") == 0) add_neg_terms = 0;
        else if (strcmp(*argv, "+self_impl") == 0) add_self_impl = 1;
        else if (strcmp(*argv, "-self_impl") == 0) add_self_impl = 0;
        argv++;
    }

    static char buffer[1024];
    if(!fgets(buffer, sizeof(buffer), stdin)) return 1;
    
    parser_t parser;
    Parser_Init(&parser, buffer);
    expr_t *goal = Parser_ReadExpr(&parser);

    
    ExtractSubformulas(goal, 0);
    if (add_neg_terms) ExtractSubformulas(goal, 1);

    if (add_self_impl) {
        expr_t* self_impl = Expr_Implies(goal, goal);
        AddTerm(self_impl);
    }

    printf("TERMS:\n");
    for (terms_set_itr it = terms_set_first(&terms); !terms_set_is_end(it); it = terms_set_next(it)) {
        printf("    "); Expr_Print(it.data->key); printf("\n");
    }
    printf("\n");

    FILE *fptr = fopen(filename, "r");
    if (fptr == NULL) {
        printf("failed to open file");
        return 1;
    }

    while (1) {
        static char line[128];
        char *res = fgets(line, sizeof(line), fptr);
        
        if (res == NULL) break;
        if (line[0] == '#') {
            continue;
        }
        
        Parser_Init(&parser, line);
        expr_t *ax = Parser_ReadExpr(&parser);
        InstantiateAxiom(ax);
    }

    fclose(fptr);

    true_expr_t *res = RunInference(goal);
    if (res != NULL && print_history) PrintHistory(res);

    return !res;
}