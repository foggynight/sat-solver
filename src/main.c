#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define AST_IMPL
#include "AST.h"

#define DA_IMPL
#include "DA.h"

#define EVAL_IMPL
#include "eval.h"

#define PARSER_IMPL
#include "parser.h"

// TODO: About a MiB, good enough?
#define INPUT_BUFSIZE 1048576

// Search for SAT solution given AST of expression and array of variables. Uses
// brute force by simply walking through each possible set of bindings linearly.
//
//   `all_solutions`: If true, return all solutions, else just first.
DA_DA_Bind solve_brute_force(
    AST *ast,
    const DA_char *vars,
    bool all_solutions)
{
    DA_DA_Bind solutions = {0};

    DA_Bind binds = binds_zero(vars);
    for (size_t i = 0; i < (1u << vars->count); ++i) {
        AST *result_ast = eval_ast_binds(ast, &binds);
        const bool result = AST_to_bool(result_ast);
        AST_free(result_ast);

        if (result) {
            const DA_Bind solution = binds_copy(&binds);
            DA_APPEND(solutions, solution);
            if (!all_solutions) { break; }
        }

        binds_inc(&binds);
    }

    return solutions;
}

int main(void) {
    char input_buffer[INPUT_BUFSIZE];
    fread(input_buffer, 1, sizeof input_buffer, stdin);
    if (getchar() != EOF) {
        fprintf(stderr, "error: unread input remaining\n");
        return 1;
    }

    DA_Token toks = lex_string(input_buffer);

    DA_char vars = {0};
    AST *ast = parse_tokens(&toks, &vars);

    printf("Expression: ");
    AST_print(ast);
    putchar('\n');

    printf("Variables: ");
    for (size_t i = 0; i < vars.count; ++i) {
        printf(" %c", vars.items[i]);
    }
    putchar('\n');

    DA_DA_Bind solutions = solve_brute_force(ast, &vars, true);

    printf("Solutions:\n");
    for (size_t i = 0; i < solutions.count; ++i) {
        DA_Bind sol = solutions.items[i];
        printf("  %ld:", i+1);
        for (size_t j = 0; j < sol.count; ++j) {
            printf(" (%c %d)", sol.items[j].var, sol.items[j].val);
        }
        putchar('\n');
    }

    return 0;
}
