#include "CNF.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "AST.h"
#include "DA.h"
#include "util.h"

CNF_Root *CNF_Root_alloc(void) {
    return calloc(1, sizeof(CNF_Root));
}

void CNF_Root_free(CNF_Root *root) {
    for (size_t i = 0; i < root->count; ++i) {
        free(root->items[i].items);
    }
    free(root);
}

static CNF_Var CNF_Var_from_AST(
    const AST *ast,
    const DA_Var *ast_vars,
    CNF_Var *out_cnf_var)
{
    bool negated;
    Var ast_var;
    if (ast->kind == AST_VAR) {
        negated = false;
        ast_var = ast->token.var;
    } else { // AST_OP (negation)
        negated = true;
        ast_var = ast->children->ast->token.var;
    }
    size_t ast_var_i;
    const bool ast_var_found = vars_find(ast_vars, ast_var, &ast_var_i);
    if (!ast_var_found) { return false; }

    if (ast_var_i > INT64_MAX - 1) {
        error_msg("CNF_from_AST: variable index too large: %ld",
                  ast_var_i);
        exit(1); // TODO: Handle this error better.
    }
    CNF_Var var = (CNF_Var)(ast_var_i + 1);
    *out_cnf_var = negated ? -var : var;
    return true;
}

CNF_Root *CNF_Root_from_AST(const AST *ast, const DA_Var *ast_vars) {
    if (!AST_is_CNF(ast)) {
        error_msg("CNF_from_AST: input AST is not CNF");
        return NULL;
    }

    CNF_Root *root = CNF_Root_alloc();
    if (!root) {
        error_msg("CNF_from_AST: failed to allocate CNF_Root");
        return NULL;
    }

    if (!AST_is_and(ast)) {
        CNF_Var cnf_var;
        const bool cnf_var_found = CNF_Var_from_AST(ast, ast_vars, &cnf_var);
        assert(cnf_var_found);
        CNF_Clause clause = {0};
        DA_APPEND(clause, cnf_var);
        DA_APPEND(*root, clause);
        return root;
    }

    for (AST_list *walk_clause = ast->children;
         walk_clause != NULL;
         walk_clause = walk_clause->next)
    {
        const AST *walk_ast = walk_clause->ast;
        CNF_Clause clause = {0};

        if (!AST_is_or(walk_ast)) {
            CNF_Var cnf_var;
            const bool cnf_var_found = CNF_Var_from_AST(
                walk_ast, ast_vars, &cnf_var);
            assert(cnf_var_found);
            CNF_Clause clause = {0};
            DA_APPEND(clause, cnf_var);
            DA_APPEND(*root, clause);
        }

        // NOTE: walk_var can be either: variable, negated variable.
        else {
            for (AST_list *walk_var = walk_ast->children;
                 walk_var != NULL;
                 walk_var = walk_var->next)
            {
                CNF_Var cnf_var;
                const bool cnf_var_found = CNF_Var_from_AST(
                    walk_var->ast, ast_vars, &cnf_var);
                assert(cnf_var_found);
                DA_APPEND(clause, cnf_var);
            }
            DA_APPEND(*root, clause);
        }
    }

    return root;

// TODO: Handle error from CNF_Var_from_AST.
//fail:
//    CNF_Root_free(root);
//    return NULL;
}

void CNF_Root_print(CNF_Root *root) {
    if (!root) {
        printf("NULL");
        return;
    }

    for (size_t i = 0; i < root->count; ++i) {
        const CNF_Clause clause = root->items[i];
        putchar('(');
        for (size_t j = 0; j < clause.count; ++j) {
            if (j > 0) { printf(" + "); }
            printf("%ld", clause.items[j]);
        }
        putchar(')');
    }
}
