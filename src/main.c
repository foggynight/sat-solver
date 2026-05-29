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
    for (size_t i = 0; i < (1 << vars->count); ++i) {
        const AST *result_ast = eval_ast_binds(ast, &binds);
        const bool result = AST_to_bool(result_ast);

        if (result) {
            DA_Bind solution = binds_copy(&binds);
            DA_APPEND(solutions, solution);
            if (!all_solutions) { break; }
        }

        binds_inc(&binds);
    }

    return solutions;
}

int main(void) {
    //DA_Token toks = lex_string("a + b + c");
    //DA_Token toks = lex_string("a * b + c * d");
    //DA_Token toks = lex_string("a + b * (c + d)");
    //DA_Token toks = lex_string("a + (b * c) + d");
    //DA_Token toks = lex_string("a + -(bc -- d * e)");
    //DA_Token toks = lex_string("a + bc + d * e");
    //DA_Token toks = lex_string("(a)");
    //DA_Token toks = lex_string("-a");
    //DA_Token toks = lex_string("(a)(-b)(c) + abc");
    //DA_Token toks = lex_string("a + b");
    //DA_Token toks = lex_string("a + b c");
    //DA_Token toks = lex_string("-(-a * -b)");
    //DA_Token toks = lex_string("(a + -b)(a + c)(b + c)(a + b + c)");

    //for (size_t i = 0; i < toks.count; ++i) {
    //    const Token tok = toks.items[i];
    //    printf("Token %ld: kind=%d chr=%c\n", i+1, tok.kind, tok.chr);
    //}

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
