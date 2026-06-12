#include "sat_solver.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "AST.h"
#include "DA.h"
#include "util.h"

// Search for SAT solution given AST of expression and array of variables. Uses
// brute force by simply walking through each possible set of bindings linearly.
DA_Solution solve_brute_force(
    const AST *ast,
    const DA_Var *vars,
    bool first_solution)
{
    DA_Solution solutions = {0};
    DA_Bind binds = binds_zero(vars);

    printf("Checking Binds:\n");
    for (size_t i = 0; i < (1u << vars->count); ++i) {
        printf("  %ld: ", i);
        binds_print(&binds);
        putchar('\n');
        AST *result_ast = AST_eval_binds(ast, &binds);
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
//static void unit_propagation(AST *ast, const DA_Var *vars) {
//
//}

typedef enum {
    POLARITY_NULL,     // Variable not in expression, skip.
    POLARITY_CONFLICT, // Instances with opposite polarities, not pure.
    POLARITY_TRUE,     // Variable only true polarity, pure.
    POLARITY_FALSE,    // Variable only false polarity, pure.
} Polarity;

static Polarity polarity_invert(Polarity pol) {
    switch (pol) {
    case POLARITY_NULL:     return POLARITY_NULL;
    case POLARITY_CONFLICT: return POLARITY_CONFLICT;
    case POLARITY_TRUE:     return POLARITY_FALSE;
    case POLARITY_FALSE:    return POLARITY_TRUE;
    }
    assert(0 && "unreachable");
    __builtin_unreachable();
}

static Polarity ple_get_polarity(const AST *ast, Var var) {
    switch (ast->kind) {
    case AST_VAR:
        return (strcmp(ast->token.var, var) == 0) ? POLARITY_TRUE : POLARITY_NULL;

    case AST_OP:
        assert(ast->children != NULL);

        if (ast->token.kind == TOK_MINUS) {
            assert(ast->children->next == NULL);
            return polarity_invert(ple_get_polarity(ast->children->ast, var));
        }

        else if (ast->token.kind == TOK_STAR || ast->token.kind == TOK_PLUS) {
            assert(ast->children->next != NULL);
            Polarity ast_polarity = POLARITY_NULL;
            AST_list *child_list = ast->children;
            do {
                const Polarity child_polarity =
                    ple_get_polarity(child_list->ast, var);
                if (child_polarity == POLARITY_NULL) {
                    continue;
                } else if (child_polarity == POLARITY_CONFLICT) {
                    return POLARITY_CONFLICT;
                } else if (ast_polarity == POLARITY_NULL) {
                    ast_polarity = child_polarity;
                } else if (ast_polarity != child_polarity) {
                    return POLARITY_CONFLICT;
                } else {
                    ast_polarity = child_polarity;
                }
            } while ((child_list = child_list->next) != NULL);
            return ast_polarity;
        }

        else { UNREACHABLE(); }

    default: UNREACHABLE();
    }

    UNREACHABLE();
}

// TODO: Assert AST is in CNF, which is required for this function.
static AST *ple_eliminate_clauses(const AST *ast, Var var) {
//    switch (ast->kind) {
//    case AST_VAR:
//        return (ast->token.var == var) ? AST_null() : ast;
//
//    case AST_UOP: {
//        AST *inner = ple_eliminate_clauses(ast->right, var);
//        return (inner->kind == AST_NULL) ? inner : ast;
//
//    }
//
//    case AST_BOP: break;
//
//    default:
//        // TODO: Can you get here without an error, e.g. empty expression?
//        fprintf(
//            stderr,
//            "error: failed to eliminate clauses, invalid AST kind: %d",
//            ast->kind);
//        exit(1);
//    }
    return NULL;
}

static AST *pure_literal_elimination(
    const AST *ast,
    const DA_Var *vars,
    DA_Bind *binds)
{
    puts("Eliminating pure literal clauses...");
    for (size_t i = 0; i < vars->count; ++i) {
        const Var var = vars->items[i];
        const Polarity polarity = ple_get_polarity(ast, var);
        if (polarity == POLARITY_TRUE || polarity == POLARITY_FALSE) {
            Bind *bind = binds_find(binds, var);
            bind->val = (polarity == POLARITY_TRUE);
            bind->lock = true;
            ple_eliminate_clauses(ast, var);
        }
    }
    return ast;
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
DA_Solution solve_DPLL(
    const AST *ast_original,
    const DA_Var *vars_original,
    bool first_solution)
{
    DA_Solution solutions = {0};

    AST *ast = AST_copy(ast_original);
    DA_Var vars = vars_copy(vars_original);
    DA_Bind binds = binds_zero(&vars);

    ast = pure_literal_elimination(ast, &vars, &binds);
    //printf("Final Expression: ");
    //AST_print(ast);
    //putchar('\n');

    printf("Checking Binds:\n");
    for (size_t i = 0; true; ++i) {
        printf("  %ld: ", i);
        binds_print(&binds);
        putchar('\n');
        AST *result_ast = AST_eval_binds(ast, &binds);
        const bool result = AST_to_bool(result_ast);
        AST_free(result_ast);
        if (result) {
            const Solution solution = binds_copy(&binds);
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
