#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define AST_IMPL
#include "AST.h"

#define DA_IMPL
#include "DA.h"

#define EVAL_IMPL
#include "eval.h"

#define PARSER_DIMACS_IMPL
#include "parser_DIMACS.h"

#define PARSER_STANDARD_IMPL
#include "parser_standard.h"

#define UTIL_IMPL
#include "util.h"

typedef enum {
    ALGO_NONE,
    ALGO_BRUTE,
    ALGO_DPLL,
} Algorithm;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "error: missing arguments: algorithm\n");
        return 1;
    }
    Algorithm algorithm = ALGO_NONE;
    if (strcmp(argv[1], "brute") == 0) { algorithm = ALGO_BRUTE; }
    if (strcmp(argv[1], "dpll") == 0) { algorithm = ALGO_DPLL; }
    if (algorithm == ALGO_NONE) {
        fprintf(stderr, "error: invalid algorithm argument: %s\n", argv[1]);
        return 1;
    }

    DA_Var vars = {0};
    //AST *ast = parse_standard_expr(stdin, &vars);
    AST *ast = parse_DIMACS_file(stdin, &vars);
    if (!ast) {
        fprintf(stderr, "error: failed to parse expression\n");
        return 1;
    }

    //printf("Initial Expression: ");
    printf("Expression: ");
    AST_print(ast);
    putchar('\n');

    const bool first_solution = false;
    DA_DA_Bind solutions;
    switch (algorithm) {
        case ALGO_BRUTE:
            solutions = solve_brute_force(ast, &vars, first_solution);
            break;
        case ALGO_DPLL:
            solutions = solve_DPLL(ast, &vars, first_solution);
            break;
        default:
            assert(0 && "unreachable");
            __builtin_unreachable();
    }

    AST_free(ast);
    vars_free(&vars);

    printf("Found Solutions:\n");
    for (size_t i = 0; i < solutions.count; ++i) {
        DA_Bind sol = solutions.items[i];
        printf("  %ld:", i);
        for (size_t j = 0; j < sol.count; ++j) {
            printf(" (%s %d)", sol.items[j].var, sol.items[j].val);
        }
        putchar('\n');
    }

    return 0;
}
