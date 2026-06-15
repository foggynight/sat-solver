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
#include "parser_DIMACS.h"
#include "parser_infix.h"
#include "sat_solver.h"
#include "util.h"

typedef enum {
    ALGORITHM_NONE,
    ALGORITHM_BRUTE,
    ALGORITHM_DPLL,
} Algorithm;

typedef enum {
    PARSER_NONE,
    PARSER_INFIX,
    PARSER_DIMACS,
} Parser;

void error(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    error_msg(msg, args);
    va_end(args);
    exit(1);
}

int main(int argc, char **argv) {
    Algorithm algorithm = ALGORITHM_DPLL;
    Parser parser = PARSER_INFIX;

    int opt;
    while ((opt = getopt(argc, argv, "a:p:")) != -1) {
        switch (opt) {
        case 'a':
            if      (strcasecmp(optarg, "brute") == 0) { algorithm = ALGORITHM_BRUTE; }
            else if (strcasecmp(optarg, "dpll") == 0)  { algorithm = ALGORITHM_DPLL; }
            else { error("invalid -a argument: %s", optarg); }
            break;
        case 'p':
            if      (strcasecmp(optarg, "infix") == 0)  { parser = PARSER_INFIX; }
            else if (strcasecmp(optarg, "dimacs") == 0) { parser = PARSER_DIMACS; }
            else { error("invalid -p argument: %s", optarg); }
            break;
        case '?':
            return 1;
        default:
            abort();
        }
    }
    if (algorithm == ALGORITHM_NONE) { error("missing algorithm option"); }
    if (parser == PARSER_NONE)    { error("missing parser option"); }
    printf("Parser: %s\n", (parser == PARSER_INFIX) ? "infix" : "DIMACS");
    printf("Algorithm: %s\n", (algorithm == ALGORITHM_BRUTE) ? "brute" : "DPLL");

    DA_Var vars = {0};
    AST *ast = NULL;

    CNF_Root *cnf_root = NULL;
    CNF_Var max_var = 0;

    if (parser == PARSER_INFIX) {
        AST *ast = parse_expr_infix(stdin, &vars);
        if (!ast) { error("failed to parse expression into AST"); }

        printf("Variables: ");
        printf("%s", vars.items[0]);
        for (size_t i = 1; i < vars.count; ++i) {
            printf(" %s", vars.items[i]);
        }
        newline();

        printf("Expression: ");
        AST_print(ast);
        newline();

        cnf_root = CNF_Root_from_AST(ast, &vars);
        if (!cnf_root) { error("failed to convert AST to CNF structure"); }

        max_var = vars.count;
    }

    else if (parser == PARSER_DIMACS) {
        cnf_root = parse_DIMACS_file(stdin, &max_var);
        if (!cnf_root) { error("failed to parse DIMACS file"); }
    }

    printf("CNF: ");
    CNF_Root_print(cnf_root);
    newline();

    const bool first_solution = false;
    DA_Solution solutions = {0};
    bool satisfiable;

    const char *err_msg = NULL;
    switch (algorithm) {
    case ALGORITHM_BRUTE: satisfiable = solve_brute_force(cnf_root, max_var, first_solution, &solutions); break;
    case ALGORITHM_DPLL: err_msg = solve_DPLL(ast, &vars, first_solution, &solutions); break;
    default: UNREACHABLE();
    }
    if (err_msg != NULL) { error(err_msg); }

    if (ast) { AST_free(ast); }
    if (vars.count > 0) { vars_free(&vars); }

    if (satisfiable) {
        puts("SATISFIABLE");
        printf("Found Solutions:\n");
        for (size_t i = 0; i < solutions.count; ++i) {
            Solution sol = solutions.items[i];
            printf("  %ld: ", i);
            CNF_Binds_print((CNF_Binds*)&sol);
            newline();
        }
    } else {
        puts("UNSATISFIABLE");
    }

    return 0;
}
