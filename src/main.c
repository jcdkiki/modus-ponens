#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EXPRS 100000
#define MAX_TERMS 1000

typedef enum {
    INFERENCE_AXIOM,
    INFERENCE_MODUS_PONENS,
    INFERENCE_DEDUCTION
} inference_type_t;

typedef struct {
    expr_t *e;
    inference_type_t type;
    int parents[3];
    int printed;
} true_expr_t;

true_expr_t pool[MAX_EXPRS];
int pool_count = 0;

expr_t *terms[MAX_TERMS];
int term_count = 0;

static int print_axioms = 0;
static int print_history = 1;
static int add_neg_terms = 0;
static int add_self_impl = 0;
static int less_terms = 0;

int IsExprInPool(expr_t *e)
{
    for (int i = 0; i < pool_count; i++) {
        if (Expr_Equal(pool[i].e, e)) return 1;
    }
    return 0;
}

int IsExprInTerms(expr_t *e)
{
    for (int i = 0; i < term_count; i++) {
        if (Expr_Equal(terms[i], e)) return 1;
    }
    return 0;
}

void AddToPool(expr_t* e, int parent1, int parent2, int parent3, inference_type_t type)
{
    if (pool_count < MAX_EXPRS) {
        if (!IsExprInPool(e)) {
            pool[pool_count].e = e;
            pool[pool_count].type = type;
            pool[pool_count].parents[0] = parent1;
            pool[pool_count].parents[1] = parent2;
            pool[pool_count].parents[2] = parent3;
            pool[pool_count].printed = 0;
            pool_count++;
        }
    }
    else {
        printf("POOL IS FULL!!!\n");
        exit(1);
    }
}

void AddTerm(expr_t* e)
{
    if (term_count < MAX_TERMS) {
        terms[term_count++] = e;
    }
    else {
        printf("TERM IS FULL!!\n");
        exit(1);
    }
}

void ExtractSubformulas(expr_t* e, int negate)
{
    if (e->type == EXPR_IMPLIES) {
        ExtractSubformulas(e->implies.a, negate);
        ExtractSubformulas(e->implies.b, negate);
    }
    else if (e->type == EXPR_NOT) {
        ExtractSubformulas(e->not.a, negate);
    }

    if (negate) AddTerm(Expr_Not(e));
    else AddTerm(e);
}

expr_t* Substitute(expr_t* template, expr_t* subA, expr_t* subB, expr_t* subC)
{
    if (template->type == EXPR_ATOM) {
        if (strcmp(template->atom.name, "A") == 0 && subA != NULL) return Expr_Clone(subA);
        if (strcmp(template->atom.name, "B") == 0 && subB != NULL) return Expr_Clone(subB);
        if (strcmp(template->atom.name, "C") == 0 && subC != NULL) return Expr_Clone(subC);
        printf("What the hell?\n");
    } 
    else if (template->type == EXPR_IMPLIES) {
        return Expr_Implies(
            Substitute(template->implies.a, subA, subB, subC),
            Substitute(template->implies.b, subA, subB, subC)
        );
    }
    else if (template->type == EXPR_NOT) {
        return Expr_Not(
            Substitute(template->not.a, subA, subB, subC)
        );
    }
    return NULL;
}

void PrintAxiom(int i, int j, int k, int expr_idx)
{
    printf("       A = ");
    Expr_Print(terms[i]);
    printf(", B = ");
    Expr_Print(terms[j]);
    printf(", C = ");
    Expr_Print(terms[k]);
    printf("\n");

    printf("  %3d ", expr_idx);
    Expr_Print(pool[expr_idx].e);
    printf("\n\n");
}

void InstantiateAxioms(expr_t* ax)
{
    if (print_axioms) {
        printf("Axiom: ");
        Expr_Print(ax);
        printf("\n");
    }
    
    for (int i = 0; i < term_count; i++) {
        for (int j = 0; j < term_count; j++) {
            for (int k = 0; k < term_count; k++) {
                expr_t *instance = Substitute(ax, terms[i], terms[j], terms[k]);
                
                if (IsExprInPool(instance)) {
                    Expr_Free(instance);
                }
                else {
                    AddToPool(instance, i, j, k, INFERENCE_AXIOM);
                    if (print_axioms) {
                        PrintAxiom(i, j, k, pool_count - 1);
                    }
                }
            }
        }
    }
}

expr_t *TryModusPonens(expr_t *a_impl_b, expr_t *a)
{
    if (a_impl_b->type == EXPR_IMPLIES) {
        if (Expr_Equal(a_impl_b->implies.a, a)) {
            return a_impl_b->implies.b;
        }
    }
    return NULL;
}

void PrintModusPonens(int i, int j, int res_idx)
{
    printf("%5d ", i);
    Expr_Print(pool[i].e);
    printf(",\n%5d  ", j); 
    int len = Expr_Print(pool[j].e);
    printf("\n%5d ", res_idx);
    while (len--) putc(' ', stdout);
    printf("  |- ");
    Expr_Print(pool[res_idx].e);
    printf("\n\n");
}

int hyp_count = 0;
expr_t *hypotheses[100];

int Prove(expr_t* target, int depth)
{
    for (int i = 0; i < depth*2 - 2; i++) putc(' ', stdout);
    printf("try prove "); Expr_Print(target); printf("\n");

    for (int i = 0; i < hyp_count; i++) {
        if (Expr_Equal(hypotheses[i], target)) {
            for (int i = 0; i < depth*2; i++) putc(' ', stdout);
            printf("already assumed: "); Expr_Print(target); printf("\n");
            goto proved;
        }
    }

    if (target->type == EXPR_IMPLIES) {
        expr_t* left = target->implies.a;
        expr_t* right = target->implies.b;
        
        for (int i = 0; i < depth*2; i++) putc(' ', stdout);
        printf("assume "); Expr_Print(left); printf("\n");
        
        hypotheses[hyp_count] = left;
        hyp_count++;
        int result = Prove(right, depth + 1);
        hyp_count--;

        if (result) goto proved;
    }

    for (int i = 0; i < hyp_count; i++) {
        if (hypotheses[i]->type == EXPR_IMPLIES && Expr_Equal(hypotheses[i]->implies.b, target)) {
            for (int i = 0; i < depth*2; i++) putc(' ', stdout);
            printf("already assumed "); Expr_Print(hypotheses[i]); printf("\n");
            if (Prove(hypotheses[i]->implies.a, depth + 1)) {
                goto proved;
            }
        }
    }

    for (int i = 0; i < depth*2 - 2; i++) putc(' ', stdout);
    printf("????\n");
    return 0;

proved:
    for (int i = 0; i < depth*2 - 2; i++) putc(' ', stdout);
    printf("TRUE!\n");
    return 1;
}

int RunInference(expr_t* goal)
{
    int new_found = 1;
    int cycle = 0;
    
    while (new_found) {
        new_found = 0;
        int current_cnt = pool_count;
        cycle++;
        
        printf("CYCLE %d\n", cycle );
        
        for (int i = 0; i < current_cnt; i++) {
            for (int j = 0; j < current_cnt; j++) {
                
                expr_t *res = TryModusPonens(pool[i].e, pool[j].e);
                
                if (res) {
                    if (!IsExprInPool(res)) {
                        AddToPool(res, i, j, -1, INFERENCE_MODUS_PONENS);
                        new_found = 1;
                        
                        if (Expr_Equal(res, goal)) {
                            printf("GOAL FOUND!\n");
                            return 1;
                        }
                    }
                }
            }
        }
    }

    return Prove(goal, 1);
}

void PrintHistory(int idx)
{
    if (idx == -1) return;
    true_expr_t *te = &pool[idx];
    if (te->printed) return;
    te->printed = 1;

    switch (te->type) {
    case INFERENCE_AXIOM:
        PrintAxiom(te->parents[0], te->parents[1], te->parents[2], idx);
        break;
    case INFERENCE_MODUS_PONENS:
        PrintHistory(te->parents[0]);
        PrintHistory(te->parents[1]);
        PrintModusPonens(te->parents[0], te->parents[1], idx);
        break;
    case INFERENCE_DEDUCTION:
        printf("%5d DEDUCTED ", idx);
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
        else if (strcmp(*argv, "+lt") == 0) less_terms = 1;
        else if (strcmp(*argv, "-lt") == 0) less_terms = 0;
        argv++;
    }

    static char buffer[1024];
    if(!fgets(buffer, sizeof(buffer), stdin)) return 1;
    
    parser_t parser;
    Parser_Init(&parser, buffer);
    expr_t *goal = Parser_ReadExpr(&parser);

    if (less_terms) {
        AddTerm(Expr_Atom("A"));
        AddTerm(Expr_Atom("B"));
        AddTerm(Expr_Atom("C"));
    }
    else {
        ExtractSubformulas(goal, 0);
        if (add_neg_terms) ExtractSubformulas(goal, 1);
    }

    if (add_self_impl) {
        expr_t* self_impl = Expr_Implies(goal, goal);
        AddTerm(self_impl);
    }

    for (int i=0; i<term_count; i++) {
        printf("  T%d: ", i); Expr_Print(terms[i]); printf("\n");
    }
    printf("\n");

    FILE *fptr = fopen(filename, "r");
    if (fptr == NULL) {
        printf("failed to open file");
        return 1;
    }

    while (1) {
        char *line = NULL;
        size_t n;
        ssize_t res = getline(&line, &n, fptr);
        
        if (res == -1) break;
        if (line[0] == '#') continue;

        Parser_Init(&parser, line);
        expr_t *ax = Parser_ReadExpr(&parser);
        InstantiateAxioms(ax);
        free(line);
    }

    fclose(fptr);

    int res = RunInference(goal);
    if (res && print_history) PrintHistory(pool_count - 1);

    return !res;
}