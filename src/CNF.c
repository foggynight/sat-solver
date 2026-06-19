#include "CNF.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "AST.h"
#include "DA.h"
#include "util.h"

size_t CNF_Clause_len(const CNF_Clause *clause) {
    assert(clause);
    return clause->count;
}

bool CNF_Clause_contains_var(const CNF_Clause *clause, CNF_Var var) {
    assert(clause);
    assert(var >= 0);
    return (size_t)var < clause->count;
}

CNF_Lit CNF_Clause_index_lits(const CNF_Clause *clause, size_t index) {
    assert(clause);
    assert(index < clause->count);
    return clause->items[index];
}

void CNF_Clause_print(const CNF_Clause *clause) {
    putchar('(');
    for (size_t j = 0; j < clause->count; ++j) {
        if (j > 0) { printf(" "); }
        printf("%" PRId64, clause->items[j]);
    }
    putchar(')');
}

void CNF_Clause_append_lit(CNF_Clause *clause, CNF_Lit *lit) {
    DA_APPEND(*clause, *lit);
}

CNF_Root *CNF_Root_alloc(void) {
    return calloc(1, sizeof(CNF_Root));
}

void CNF_Root_free(CNF_Root *root) {
    for (size_t i = 0; i < root->count; ++i) {
        free(root->items[i].items);
    }
    free(root);
}

size_t CNF_Root_len(const CNF_Root *root) {
    assert(root);
    return root->count;
}

CNF_Clause CNF_Root_index_clauses(const CNF_Root *root, size_t index) {
    assert(root);
    return root->items[index];
}

void CNF_Root_append_clause(CNF_Root *root, CNF_Clause *clause) {
    assert(root);
    DA_APPEND(*root, *clause);
}

bool CNF_Root_eval_with_binds(const CNF_Root *root, const CNF_Binds *binds) {
    for (size_t i = 0; i < root->count; ++i) {
        const CNF_Clause clause = root->items[i];
        bool clause_true = false;
        for (size_t j = 0; j < clause.count; ++j) {
            const CNF_Lit lit = clause.items[j];
            const CNF_Var var = imaxabs(lit);
            const bool valid_bound =
                CNF_Binds_contains_var(binds, var)
                && CNF_Binds_is_bound(binds, var);
            assert(valid_bound && "variable out of range or unbound");
            if (var < 0) {  // negated variable
                if (CNF_Binds_is_bound_false(binds, var)) {
                    clause_true = true;
                    break;
                }
            } else if (var > 0) {  // affirmative variable
                if (CNF_Binds_is_bound_true(binds, var)) {
                    clause_true = true;
                    break;
                }
            } else {  // zero is invalid variable
                UNREACHABLE();
            }
        }
        if (!clause_true) { return false; }
    }
    return true;
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
        error_msg("CNF_from_AST: variable index too large: %" PRId64,
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

    // Check if AST is a variable or single disjunction.
    if (!AST_is_and(ast)) {
        CNF_Var cnf_var;
        const bool cnf_var_found = CNF_Var_from_AST(ast, ast_vars, &cnf_var);
        assert(cnf_var_found);
        CNF_Clause clause = {0};
        DA_APPEND(clause, (CNF_Lit)cnf_var); // TODO: Remove (need for) conversion.
        DA_APPEND(*root, clause);
        return root;
    }

    // AST is a conjunction of disjunctions, CNF.
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
            DA_APPEND(clause, (CNF_Lit)cnf_var); // TODO: Remove (need for) conversion.
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
                DA_APPEND(clause, (CNF_Lit)cnf_var); // TODO: Remove (need for) conversion.
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

void CNF_Root_print(const CNF_Root *root) {
    if (!root) {
        printf("NULL");
        return;
    }
    for (size_t i = 0; i < root->count; ++i) {
        const CNF_Clause clause = root->items[i];
        CNF_Clause_print(&clause);
    }
}

CNF_Binds CNF_Binds_make_vars(CNF_Var max_var) {
    const size_t arr_size = max_var + 1;  // Account for invalid zero variable.
    CNF_Binds binds;
    binds.items = calloc(arr_size, sizeof(CNF_Bind));
    assert(binds.items != NULL);
    binds.count = arr_size;
    binds.capacity = arr_size;
    return binds;
}

CNF_Binds CNF_Binds_make_zeros(CNF_Var max_var) {
    CNF_Binds binds = CNF_Binds_make_vars(max_var);
    for (size_t i = 0; i < binds.count; ++i) {
        binds.items[i].bound = true;
    }
    return binds;
}

CNF_Binds CNF_Binds_copy(const CNF_Binds *binds) {
    const size_t items_size = binds->count * sizeof(*(binds->items));
    CNF_Binds copy = *binds;
    copy.items = malloc(items_size);
    assert(copy.items != NULL);
    memcpy(copy.items, binds->items, items_size);
    return copy;
}

// NOTE: Assumes all variables are bound, such as when binds object created by
//       calling `CNF_Binds_make_zeros`.
bool CNF_Binds_inc(CNF_Binds *binds) {
    bool carry = true;
    for (size_t i = 1; i <= binds->count; ++i) {
        if (binds->items[i].pure == true) { continue; }
        const bool next_val = (binds->items[i].val != carry);
        carry = (binds->items[i].val && carry);
        binds->items[i].val = next_val;
        if (!carry) { break; }
    }
    return !carry;  // carry == true => overflow
}

void CNF_Binds_free(CNF_Binds *binds) {
    assert(binds != NULL);
    free(binds->items);
}

bool CNF_Binds_contains_var(const CNF_Binds *binds, CNF_Var var) {
    return (size_t)imaxabs(var) < binds->count;
}

bool CNF_Binds_is_bound(const CNF_Binds *binds, CNF_Var var) {
    return binds->items[var].bound;
}

bool CNF_Binds_is_bound_to(const CNF_Binds *binds, CNF_Var var, bool val) {
    //const bool negated = var < 0;
    //const CNF_Bind bind = binds->items[negated ? -var : var];
    const CNF_Bind bind = binds->items[var];
    return bind.bound && bind.val == val;
}

bool CNF_Binds_is_bound_true(const CNF_Binds *binds, CNF_Var var) {
    return CNF_Binds_is_bound_to(binds, var, true);
}

bool CNF_Binds_is_bound_false(const CNF_Binds *binds, CNF_Var var) {
    return CNF_Binds_is_bound_to(binds, var, false);
}

// TODO: pure info
void CNF_Bind_print(CNF_Var var, const CNF_Bind *bind) {
    if (bind->bound) {
        printf("(%" PRId64 " %c)", var, bind->val ? 'T' : 'F');
    } else {
        printf("(%" PRId64 " ?)", var);
    }
}

void CNF_Binds_print(const CNF_Binds *binds) {
    putchar('[');
    for (CNF_Var var = 1; (size_t)var < binds->count; ++var) {
        if (var > 1) { fputs(", ", stdout); }
        CNF_Bind_print(var, &(binds->items[var]));
    }
    putchar(']');
}
