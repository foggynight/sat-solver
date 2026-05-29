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
    const AST *ast,
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

    binds_free(&binds);
    return solutions;
}

// TODO
//void unit_propagation(AST *ast, const DA_char *vars) {
//
//}

typedef enum {
    POLARITY_NULL,     // Variable not in expression, skip.
    POLARITY_CONFLICT, // Instances with opposite polarities, not pure.
    POLARITY_TRUE,     // Variable only true polarity, pure.
    POLARITY_FALSE,    // Variable only false polarity, pure.
} Polarity;

Polarity polarity_invert(Polarity pol) {
    switch (pol) {
    case POLARITY_NULL:     return POLARITY_NULL;
    case POLARITY_CONFLICT: return POLARITY_CONFLICT;
    case POLARITY_TRUE:     return POLARITY_FALSE;
    case POLARITY_FALSE:    return POLARITY_TRUE;
    }
    assert(0 && "unreachable");
    __builtin_unreachable();
}

Polarity ple_get_polarity(const AST *ast, char var) {
    if (ast->kind == AST_VAR) {
        return (ast->token.chr == var) ? POLARITY_TRUE : POLARITY_NULL;
    }

    else if (ast->kind == AST_UOP) {
        assert(ast->token.kind == TOK_MINUS);
        return polarity_invert(ple_get_polarity(ast->right, var));
    }

    else if (ast->kind == AST_BOP) {
        const Polarity left = ple_get_polarity(ast->left, var);
        const Polarity right = ple_get_polarity(ast->right, var);
        if (left == POLARITY_NULL) { return right; }
        if (right == POLARITY_NULL) { return left; }
        return (left == right) ? left : POLARITY_CONFLICT;
    }

    else {
        // TODO: Can you get here without an error, e.g. empty expression?
        fprintf(
            stderr,
            "error: failed to get polarity, invalid AST kind: %d",
            ast->kind);
        exit(1);
    }
}

void ple_eliminate_clauses(AST **ast, char var) {

}

void pure_literal_elimination(AST **ast, const DA_char *vars, DA_Bind *binds) {
    for (size_t i = 0; i < vars->count; ++i) {
        const char var = vars->items[i];
        const Polarity polarity = ple_get_polarity(*ast, var);
        if (polarity == POLARITY_TRUE || polarity == POLARITY_FALSE) {
            // TODO: Mark this variable as locked to a specific value in binds.
            ple_eliminate_clauses(ast, var);
        }
    }
}

// TODO: Assert AST is in CNF.
//
// DPLL: Davis-Putnam-Logemann-Loveland Algorithm, unit propagation and pure
// literal elimination. Input AST must be in CNF.
//
//   `all_solutions`: If true, return all solutions, else just first.
DA_DA_Bind solve_DPLL(
    const AST *ast_original,
    const DA_char *vars_original,
    bool all_solutions)
{
    DA_DA_Bind solutions = {0};

    AST *ast = AST_copy(ast_original);
    DA_char vars = vars_copy(vars_original);
    DA_Bind binds = binds_zero(&vars);

    AST_print(ast);
    pure_literal_elimination(&ast, &vars, &binds);
    AST_print(ast);

    for (size_t i = 0; i < (1u << vars.count); ++i) {
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

    binds_free(&binds);
    vars_free(&vars);
    AST_free(ast);

    return solutions;
}

int main(void) {
    char input_buffer[INPUT_BUFSIZE];
    size_t cnt = fread(input_buffer, 1, sizeof input_buffer, stdin);
    if (getchar() != EOF) {
        fprintf(stderr, "error: unread input remaining\n");
        return 1;
    }
    input_buffer[cnt - 1] = '\0';  // destroy newline
    printf("String: \"%s\"\n", input_buffer);

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

    //DA_DA_Bind solutions = solve_brute_force(ast, &vars, true);
    DA_DA_Bind solutions = solve_DPLL(ast, &vars, true);

    AST_free(ast);
    vars_free(&vars);

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
