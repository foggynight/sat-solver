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
    return clause->lits.count;
}

CNF_Lit CNF_Clause_index_lits(const CNF_Clause *clause, size_t index) {
    assert(clause);
    assert(index < clause->lits.count);
    return clause->lits.items[index];
}

bool CNF_Clause_contains_lit(const CNF_Clause *clause, CNF_Lit lit) {
    assert(clause);
    for (size_t i = 0; i < clause->lits.count; ++i) {
        if (lit == clause->lits.items[i]) {
            return true;
        }
    }
    return false;
}

void CNF_Clause_append_lit(CNF_Clause *clause, CNF_Lit *lit) {
    DA_APPEND(clause->lits, *lit);
}

void CNF_Clause_print(const CNF_Clause *clause) {
    putchar('(');
    for (size_t j = 0; j < clause->lits.count; ++j) {
        if (j > 0) { printf(" "); }
        printf("%" PRId64, clause->lits.items[j]);
    }
    putchar(')');
}

CNF_Root *CNF_Root_alloc(void) {
    return calloc(1, sizeof(CNF_Root));
}

void CNF_Root_free(CNF_Root *root) {
    for (size_t i = 0; i < root->count; ++i) {
        free(root->items[i].lits.items);
    }
    free(root);
}

size_t CNF_Root_len(const CNF_Root *root) {
    assert(root);
    return root->count;
}

CNF_Clause *CNF_Root_index_clauses(const CNF_Root *root, size_t index) {
    assert(root);
    assert(index < root->count);
    return &(root->items[index]);
}

void CNF_Root_append_clause(CNF_Root *root, CNF_Clause *clause) {
    assert(root);
    DA_APPEND(*root, *clause);
}

bool CNF_Root_eval_with_binds(const CNF_Root *root, const CNF_Binds *binds) {
    for (size_t i = 0; i < root->count; ++i) {
        const CNF_Clause clause = root->items[i];
        if (clause.is_deleted) { continue; }
        bool clause_true = false;
        for (size_t j = 0; j < clause.lits.count; ++j) {
            const CNF_Lit lit = clause.lits.items[j];
            const CNF_Var var = imaxabs(lit);
            const bool valid_bound =
                CNF_Binds_contains_var(binds, var)
                && CNF_Binds_is_bound(binds, var);
            assert(valid_bound && "variable out of range or unbound");
            if (lit < 0) {  // negated variable
                if (CNF_Binds_is_bound_false(binds, var)) {
                    clause_true = true;
                    break;
                }
            } else if (lit > 0) {  // affirmative variable
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

// Convert an AST of form literal into a CNF_Lit, returns 0 on error.
static CNF_Lit CNF_Lit_from_AST(const AST *ast, const DA_Var *ast_vars) {
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
    if (!ast_var_found) { return 0; }

    if (ast_var_i > INT64_MAX - 1) {
        error_msg("CNF_from_AST: variable index too large: %" PRId64,
                  ast_var_i);
        return 0;
    }
    CNF_Var var = (CNF_Var)(ast_var_i + 1);
    return negated ? -var : var;
}

// Convert an AST of form: [literal, disj of literals] into a CNF_Clause.
static bool CNF_Clause_from_AST(
    const AST *ast, const DA_Var *ast_vars, CNF_Clause *out_clause)
{
    if (AST_is_lit(ast)) {
        const CNF_Lit cnf_lit = CNF_Lit_from_AST(ast, ast_vars);
        if (cnf_lit == 0) {
            error_msg("CNF_Root_from_AST: failed to extract CNF_Lit from AST");
            return false;
        }
        DA_APPEND(out_clause->lits, cnf_lit);
    }

    else if (AST_is_or(ast)) {
        for (AST_list *walk = ast->children; walk != NULL; walk = walk->next) {
            const CNF_Lit cnf_lit = CNF_Lit_from_AST(walk->ast, ast_vars);
            if (cnf_lit == 0) {
                error_msg("CNF_Root_from_AST: failed to extract CNF_Lit from AST");
                return false;
            }
            DA_APPEND(out_clause->lits, cnf_lit);
        }
    }

    else { UNREACHABLE(); }

    return true;
}

// Convert an AST in CNF (form) into a CNF_Root.
CNF_Root *CNF_Root_from_AST(const AST *ast, const DA_Var *ast_vars) {
    if (!AST_is_CNF(ast)) {
        error_msg("CNF_Root_from_AST: input AST is not CNF");
        return NULL;
    }

    CNF_Root *root = CNF_Root_alloc();
    if (!root) {
        error_msg("CNF_Root_from_AST: failed to allocate CNF_Root");
        return NULL;
    }

    if (AST_is_lit(ast) || AST_is_or(ast)) {
        CNF_Clause clause = {0};
        const bool success = CNF_Clause_from_AST(ast, ast_vars, &clause);
        if (!success) { goto fail; }
        DA_APPEND(*root, clause);
    }

    else if (AST_is_and(ast)) {
        for (AST_list *walk = ast->children; walk != NULL; walk = walk->next) {
            CNF_Clause clause = {0};
            const bool success = CNF_Clause_from_AST(walk->ast, ast_vars, &clause);
            if (!success) { goto fail; }
            DA_APPEND(*root, clause);
        }
    }

    else { UNREACHABLE(); }

    return root;

fail:
    CNF_Root_free(root);
    return NULL;
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

void CNF_Binds_free(CNF_Binds *binds) {
    assert(binds != NULL);
    free(binds->items);
}

// NOTE: Assumes all variables are bound, such as when binds object created by
//       calling `CNF_Binds_make_zeros`.
// Increment bindings in big-endian order. e.g. ABC: 010 -> 011 -> 100
bool CNF_Binds_increment(CNF_Binds *binds) {
    bool carry = true;
    for (size_t i = binds->count - 1; i > 0; --i) {
        const bool next_val = (binds->items[i].val != carry);
        carry = (binds->items[i].val && carry);
        binds->items[i].val = next_val;
        if (!carry) { break; }
    }
    return !carry;  // carry == true => overflow
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
