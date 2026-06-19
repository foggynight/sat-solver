////////////////////////////////////////////////////////////////////////////////
//
//  Conjunctive Normal Form Expression Structure
//
//  Copyright (C) 2026 Robert Coffey
//
////////////////////////////////////////////////////////////////////////////////

#ifndef CNF_H
#define CNF_H

#include <stdint.h>

#include "AST.h"
#include "DA.h"

#define CNF_VAR_NULL (CNF_Var)0

// Variable represents only variable, not polarity.
typedef int64_t CNF_Var;
typedef DA(CNF_Var) DA_CNF_Var;

// Literal represents variable and polarity.
typedef CNF_Var CNF_Lit;
typedef DA(CNF_Lit) DA_CNF_Lit;

typedef DA_CNF_Lit CNF_Clause;  // TODO: Convert to struct containing vars and deleted flag.
typedef DA(CNF_Clause) CNF_Root;

typedef struct {
    bool val;
    bool bound;
    bool pure;
} CNF_Bind;
typedef DA(CNF_Bind) DA_CNF_Bind;
typedef DA_CNF_Bind CNF_Binds;

size_t CNF_Clause_len(const CNF_Clause *clause);
bool CNF_Clause_contains_var(const CNF_Clause *clause, CNF_Var var);
CNF_Lit CNF_Clause_index_lits(const CNF_Clause *clause, size_t index);
void CNF_Clause_append_lit(CNF_Clause *clause, CNF_Lit *lit);
void CNF_Clause_print(const CNF_Clause *clause);

CNF_Root *CNF_Root_alloc(void);
void CNF_Root_free(CNF_Root *root);
size_t CNF_Root_len(const CNF_Root *root);
CNF_Clause CNF_Root_index_clauses(const CNF_Root *root, size_t index);
void CNF_Root_append_clause(CNF_Root *root, CNF_Clause *clause);
bool CNF_Root_eval_with_binds(const CNF_Root *root, const CNF_Binds *binds);
CNF_Root *CNF_Root_from_AST(const AST *ast, const DA_Var *ast_vars);
void CNF_Root_print(const CNF_Root *root);

void CNF_Bind_bind_to(CNF_Binds *binds, CNF_Var var, bool val);
void CNF_Bind_print(CNF_Var var, const CNF_Bind *bind);

// TODO: Should these functions handle negated variable or the caller?
CNF_Binds CNF_Binds_make_vars(CNF_Var max_var);
CNF_Binds CNF_Binds_make_zeros(CNF_Var max_var);
CNF_Binds CNF_Binds_copy(const CNF_Binds *binds);
//CNF_Bind *CNF_Binds_index(const CNF_Binds *binds, size_t index);
bool CNF_Binds_inc(CNF_Binds *binds);
void CNF_Binds_free(CNF_Binds *binds);
bool CNF_Binds_contains_var(const CNF_Binds *binds, CNF_Var var);
bool CNF_Binds_is_bound(const CNF_Binds *binds, CNF_Var var);
bool CNF_Binds_is_bound_to(const CNF_Binds *binds, CNF_Var var, bool val);
bool CNF_Binds_is_bound_true(const CNF_Binds *binds, CNF_Var var);
bool CNF_Binds_is_bound_false(const CNF_Binds *binds, CNF_Var var);
void CNF_Binds_print(const CNF_Binds *binds);

#endif // CNF_H
