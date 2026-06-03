////////////////////////////////////////////////////////////////////////////////
//
//  Evaluator for Boolean Expressions
//
//  Copyright (C) 2026 Robert Coffey
//
////////////////////////////////////////////////////////////////////////////////

#ifndef EVAL_H
#define EVAL_H

#include "AST.h"
#include "DA.h"

typedef struct {
    Var var;   // variable
    bool val;  // bound value
    bool lock; // is variable pure
} Bind;

typedef DA(Bind) DA_Bind;
typedef DA(DA_Bind) DA_DA_Bind;

void Bind_print(const Bind *bind);

DA_Var vars_copy(const DA_Var *vars);
void vars_free(DA_Var *vars);

DA_Bind binds_zero(const DA_Var *vars);  // Bindings from vars, all false.
DA_Bind binds_copy(const DA_Bind *binds);
void binds_free(DA_Bind *binds);
Bind *binds_find(DA_Bind *binds, Var var);

// Increment variable bindings, like binary increment with first binding
// corresponding to least significant digit.
//
// Skips locked bindings as if they don't exist.
//
// Returns true when binds was modified, else false.
bool binds_inc(DA_Bind *binds);

void binds_print(const DA_Bind *binds);

// Evaluate AST to true/false given variable bindings.
AST *eval_ast_binds(const AST *ast, const DA_Bind *binds);

DA_DA_Bind solve_brute_force(
    const AST *ast,
    const DA_Var *vars,
    bool first_solution);

DA_DA_Bind solve_DPLL(
    const AST *ast_original,
    const DA_Var *vars_original,
    bool first_solution);

#endif // EVAL_H


#ifdef EVAL_IMPL
#undef EVAL_IMPL

#include <stdbool.h>
#include <string.h>

#include "AST.h"
#include "DA.h"

static AST *binds_lookup(const DA_Bind *binds, Var var) {
    for (size_t i = 0; i < binds->count; ++i) {
        Bind bind = binds->items[i];
        if (strcmp(bind.var, var) == 0) {
            return bool_to_AST(bind.val);
        }
    }
    assert(0 && "unreachable");
    __builtin_unreachable();
}

DA_Var vars_copy(const DA_Var *vars) {
    assert(vars != NULL);
    DA_Var copy = *vars;
    copy.items = malloc(vars->capacity * sizeof(Var));
    assert(copy.items != NULL);
    memcpy(copy.items, vars->items, vars->count * sizeof(Bind));
    return copy;
}

void vars_free(DA_Var *vars) {
    free(vars->items);
}

void Bind_print(const Bind *bind) {
    const char *name = bind->lock ? "pure" : "bind";
    printf("(%s %s %d)", name, bind->var, bind->val);
}

DA_Bind binds_zero(const DA_Var *vars) {
    DA_Bind binds = {0};
    for (size_t i = 0; i < vars->count; ++i) {
        DA_APPEND(binds, ((Bind){ vars->items[i], false, false }));
    }
    return binds;
}

DA_Bind binds_copy(const DA_Bind *binds) {
    DA_Bind copy = *binds;
    copy.items = malloc(binds->capacity * sizeof(Bind));
    assert(copy.items != NULL);
    memcpy(copy.items, binds->items, binds->count * sizeof(Bind));
    return copy;
}

void binds_free(DA_Bind *binds) {
    free(binds->items);
}

Bind *binds_find(DA_Bind *binds, Var var) {
    for (size_t i = 0; i < binds->count; ++i) {
        if (strcmp(binds->items[i].var, var) == 0) {
            return &(binds->items[i]);
        }
    }
    return NULL;
}

bool binds_inc(DA_Bind *binds) {
    bool carry = true;
    for (size_t i = 0; i < binds->count; ++i) {
        if (binds->items[i].lock == true) { continue; }
        const bool next_val = (binds->items[i].val != carry);
        carry = (binds->items[i].val && carry);
        binds->items[i].val = next_val;
        if (!carry) { break; }
    }
    return carry;
}

void binds_print(const DA_Bind *binds) {
    bool first = true;
    for (size_t i = 0; i < binds->count; ++i) {
        if (first) {
            first = false;
        } else {
            putchar(' ');
        }
        Bind_print(&binds->items[i]);
    }
}

AST *eval_ast_binds(const AST *ast, const DA_Bind *binds) {
    switch (ast->kind) {
    case AST_TRUE: return AST_true();
    case AST_FALSE: return AST_false();

    case AST_VAR:
        return binds_lookup(binds, ast->token.var);

    case AST_UOP:
        if (*(ast->token.var) != '-') { return NULL; }
        return AST_eval_not(eval_ast_binds(ast->right, binds));

    case AST_BOP: {
        const AST *left_val = eval_ast_binds(ast->left, binds);
        const AST *right_val = eval_ast_binds(ast->right, binds);
        switch (*(ast->token.var)) {
        case '*':
            return AST_eval_and(left_val, right_val);
        case '+':
            return AST_eval_or(left_val, right_val);
        default:
            return NULL;
        }
        assert(0 && "unreachable");
        __builtin_unreachable();
    }

    default:
        return NULL;
    }

    assert(0 && "unreachable");
    __builtin_unreachable();
}

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

static void ple_eliminate_clauses(AST **ast, Var var) {
    if (ast || var) return;  // TODO: temp
}

static void pure_literal_elimination(AST **ast, const DA_Var *vars, DA_Bind *binds) {
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

#endif // EVAL_IMPL
