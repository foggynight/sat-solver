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
#include "CNF.h"
#include "DA.h"

//#define PARSER_DIMACS_IMPL
//#include "parser_DIMACS.h"

#include "parser_infix.h"
#include "sat_solver.h"
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
    error_msg(msg, args);
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
    newline();

    printf("Initial Expression: ");
    //printf("Expression: ");
    AST_print(ast);
    newline();

    CNF_Root *cnf_root = CNF_Root_from_AST(ast, &vars);
    printf("CNF: ");
    CNF_Root_print(cnf_root);
    newline();

    CNF_Binds cnf_binds = CNF_Binds_make_vars(vars.count);
    for (size_t i = 0; i < cnf_binds.count; ++i) {
        CNF_Bind *cnf_bind = &(cnf_binds.items[i]);
        cnf_bind->bound = true;
        cnf_bind->val = true;
    }
    cnf_binds.items[1].val = false;
    cnf_binds.items[2].val = true;
    printf("Binds: ");
    CNF_Binds_print(&cnf_binds);
    newline();

    bool result = CNF_Root_eval_with_binds(cnf_root, &cnf_binds);
    printf("Result: %d\n", result);

    return 0; // TODO: TEMPORARY -----------------------------------------------

    const bool first_solution = false;
    DA_Solution solutions = {0};
    const char *error_msg = NULL;
    switch (algorithm) {
    case ALGO_BRUTE: error_msg = solve_brute_force(ast, &vars, first_solution, &solutions); break;
    case ALGO_DPLL: error_msg = solve_DPLL(ast, &vars, first_solution, &solutions); break;
    default: assert(0 && "unreachable"); __builtin_unreachable();
    }

    if (error_msg != NULL) {
        printf("error: %s\n", error_msg);
        return 1;
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
        newline();
    }

    return 0;
}
