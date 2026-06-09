////////////////////////////////////////////////////////////////////////////////
//
//  SAT Solver (Algorithms: Brute Force, DPLL)
//
//  Usage: sat-solver [-a ALGORITHM] [-p PARSER]
//    ALGORITHM: brute, dpll
//    PARSER: infix, dimacs
//    Reads input expression from stdin.
//
//  Copyright (C) 2026 Robert Coffey
//
////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "AST.h"
#include "DA.h"

//#define EVAL_IMPL
//#include "eval.h"
//
//#define PARSER_DIMACS_IMPL
//#include "parser_DIMACS.h"

#include "parser_infix.h"
#include "util.h"

typedef enum {
    ALGO_NONE,
    ALGO_BRUTE,
    ALGO_DPLL,
} Algorithm;

typedef enum {
    PARS_NONE,
    PARS_INFIX,
    PARS_DIMACS,
} Parser;

void error(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    fprintf(stderr, "error: ");
    fprintf(stderr, msg, args);
    putc('\n', stderr);
    exit(1);
}

int main(int argc, char **argv) {
    Algorithm algorithm = ALGO_DPLL;
    Parser parser = PARS_INFIX;

    int opt;
    while ((opt = getopt(argc, argv, "a:p:")) != -1) {
        switch (opt) {
        case 'a':
            if      (strcasecmp(optarg, "brute") == 0) { algorithm = ALGO_BRUTE; }
            else if (strcasecmp(optarg, "dpll") == 0)  { algorithm = ALGO_DPLL; }
            else { error("invalid -a argument: %s", optarg); }
            break;
        case 'p':
            if      (strcasecmp(optarg, "infix") == 0) { parser = PARS_INFIX; }
            else if (strcasecmp(optarg, "dimacs") == 0)   { parser = PARS_DIMACS; }
            else { error("invalid -p argument: %s", optarg); }
            break;
        case '?':
            return 1;
        default:
            abort();
        }
    }
    if (algorithm == ALGO_NONE) { error("missing algorithm option"); }
    if (parser == PARS_NONE)    { error("missing parser option"); }

    DA_Var vars = {0};
    AST *ast;
    switch (parser) {
    case PARS_INFIX:    ast = parse_expr_infix(stdin, &vars); break;
    //case PARS_DIMACS: ast = parse_DIMACS_file(stdin, &vars); break;
    default: assert(0 && "unreachable"); __builtin_unreachable();
    }
    if (!ast) { error("failed to parse expression"); }

    printf("Parser: %s\n", (parser == PARS_INFIX) ? "infix" : "DIMACS");
    printf("Algorithm: %s\n", (algorithm == ALGO_BRUTE) ? "brute" : "DPLL");

    printf("Variables: ");
    printf("%s", vars.items[0]);
    for (size_t i = 1; i < vars.count; ++i) {
        printf(" %s", vars.items[i]);
    }
    putchar('\n');

    //printf("Initial Expression: ");
    printf("Expression: ");
    AST_print(ast);
    putchar('\n');

//    const bool first_solution = false;
//    DA_DA_Bind solutions;
//    switch (algorithm) {
//    case ALGO_BRUTE: solutions = solve_brute_force(ast, &vars, first_solution); break;
//    case ALGO_DPLL: solutions = solve_DPLL(ast, &vars, first_solution); break;
//    default: assert(0 && "unreachable"); __builtin_unreachable();
//    }
//
//    AST_free(ast);
//    vars_free(&vars);
//
//    printf("Found Solutions:\n");
//    for (size_t i = 0; i < solutions.count; ++i) {
//        DA_Bind sol = solutions.items[i];
//        printf("  %ld:", i);
//        for (size_t j = 0; j < sol.count; ++j) {
//            printf(" (%s %d)", sol.items[j].var, sol.items[j].val);
//        }
//        putchar('\n');
//    }
//
//    return 0;
//}

//    AST *A = AST_make_var("A");
//    AST *B = AST_make_var("B");
//    AST *C = AST_make_var("C");
//    AST *D = AST_make_var("D");
//
//    AST *F = AST_make_and();
//    AST_append(F, A);
//    AST_append(F, B);
//    AST_append(F, C);
//    AST_append(F, D);
//
//    AST *G = AST_make_and();
//    AST_append(G, D);
//    AST_append(G, C);
//    AST_append(G, B);
//    AST_append(G, A);
//
//    AST *H = AST_make_or();
//    AST_append(H, F);
//    AST_append(H, G);
//
//    AST_print(H);
}
