#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define AST_IMPL
#include "AST.h"

#define DA_IMPL
#include "DA.h"

#define EVAL_IMPL
#include "eval.h"

//#define PARSER_DIMACS_IMPL
//#include "parser_DIMACS.h"

#define PARSER_STANDARD_IMPL
#include "parser_standard.h"

#define UTIL_IMPL
#include "util.h"

#define INPUT_BUFSIZE 1048576

// Search for SAT solution given AST of expression and array of variables. Uses
// brute force by simply walking through each possible set of bindings linearly.
DA_DA_Bind solve_brute_force(
    const AST *ast,
    const DA_Var *vars,
    bool first_solution)
{
    DA_DA_Bind solutions = {0};
    DA_Bind binds = binds_zero(vars);

    printf("Checking Binds:\n");
    for (size_t i = 0; i < (1u << vars->count); ++i) {
        printf("  %ld: ", i);
        binds_print(&binds);
        putchar('\n');
        AST *result_ast = eval_ast_binds(ast, &binds);
        const bool result = AST_to_bool(result_ast);
        AST_free(result_ast);
        if (result) {
            const DA_Bind solution = binds_copy(&binds);
            DA_APPEND(solutions, solution);
            if (first_solution) { break; }
        }
        binds_inc(&binds);
    }

    binds_free(&binds);
    return solutions;
}

// TODO
//void unit_propagation(AST *ast, const DA_Var *vars) {
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

Polarity ple_get_polarity(const AST *ast, Var var) {
    if (ast->kind == AST_VAR) {
        return (strcmp(ast->token.var, var) == 0) ? POLARITY_TRUE : POLARITY_NULL;
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

void ple_eliminate_clauses(AST **ast, Var var) {
    if (ast || var) return;  // TODO: temp
}

void pure_literal_elimination(AST **ast, const DA_Var *vars, DA_Bind *binds) {
    //puts("Eliminating pure literal clauses...");
    for (size_t i = 0; i < vars->count; ++i) {
        const Var var = vars->items[i];
        const Polarity polarity = ple_get_polarity(*ast, var);
        if (polarity == POLARITY_TRUE || polarity == POLARITY_FALSE) {
            Bind *bind = binds_find(binds, var);
            bind->val = (polarity == POLARITY_TRUE);
            bind->lock = true;
            ple_eliminate_clauses(ast, var);
        }
    }
}

// TODO: Assert AST is in CNF.
//
// DPLL: Davis-Putnam-Logemann-Loveland Algorithm, unit propagation and pure
// literal elimination. Input AST must be in CNF.
//
// Note: DPLL reduces search space, thus not all solutions will be output.
//   e.g. "(A + B)(A + C)(B + -B)(C + -C)" => A is pure true, no solutions with
//   A = false will be checked and thus are not included in the output. Further,
//   "(A + B)(A + C)" => A,B,C all pure, thus only solution checked is all true.
DA_DA_Bind solve_DPLL(
    const AST *ast_original,
    const DA_Var *vars_original,
    bool first_solution)
{
    DA_DA_Bind solutions = {0};

    AST *ast = AST_copy(ast_original);
    DA_Var vars = vars_copy(vars_original);
    DA_Bind binds = binds_zero(&vars);

    pure_literal_elimination(&ast, &vars, &binds);
    //printf("Final Expression: ");
    //AST_print(ast);
    //putchar('\n');

    printf("Checking Binds:\n");
    for (size_t i = 0; true; ++i) {
        printf("  %ld: ", i);
        binds_print(&binds);
        putchar('\n');
        AST *result_ast = eval_ast_binds(ast, &binds);
        const bool result = AST_to_bool(result_ast);
        AST_free(result_ast);
        if (result) {
            const DA_Bind solution = binds_copy(&binds);
            DA_APPEND(solutions, solution);
            if (first_solution) { break; }
        }
        if (binds_inc(&binds)) { break; }
    }

    binds_free(&binds);
    vars_free(&vars);
    AST_free(ast);

    return solutions;
}

AST *parse_standard_expr(FILE *input, DA_Var *vars) {
    char input_buffer[INPUT_BUFSIZE];
    if (!fgets(input_buffer, INPUT_BUFSIZE, input)) { return NULL; }

    size_t input_len = strlen(input_buffer);
    while (input_buffer[input_len - 1] == '\n') {
        input_buffer[input_len - 1] = '\0';
        --input_len;
    }

    DA_Token toks = lex_string(input_buffer);
    AST *ast = parse_tokens(&toks, vars);

    printf("Input: \"%s\"\n", input_buffer);
    printf("Variables: ");
    printf("%s", vars->items[0]);
    for (size_t i = 1; i < vars->count; ++i) {
        printf(" %s", vars->items[i]);
    }
    putchar('\n');

    return ast;
}

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
    AST *ast = parse_standard_expr(stdin, &vars);
    //AST *ast = parse_DIMACS_file(stdin, &vars);
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
        case ALGO_BRUTE: solutions = solve_brute_force(ast, &vars, first_solution); break;
        case ALGO_DPLL:  solutions = solve_DPLL(ast, &vars, first_solution); break;
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
