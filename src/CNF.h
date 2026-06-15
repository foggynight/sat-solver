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

typedef int64_t CNF_Var;
typedef DA(CNF_Var) DA_CNF_Var;
typedef DA_CNF_Var CNF_Clause;
typedef DA(CNF_Clause) CNF_Root;

typedef struct {
    //CNF_Var var;
    bool val;
    bool bound;
    bool pure;
} CNF_Bind;

typedef DA(CNF_Bind) CNF_Binds;

DA_CNF_Var *DA_CNF_Var_from_DA_AST_Var(const DA_AST_Var *ast_vars);

CNF_Root *CNF_Root_alloc(void);
void CNF_Root_free(CNF_Root *root);
void CNF_Clause_append_var(CNF_Clause *clause, CNF_Var *var);
void CNF_Root_append_clause(CNF_Root *root, CNF_Clause *clause);
bool CNF_Root_eval_with_binds(const CNF_Root *root, const CNF_Binds *binds);
CNF_Root *CNF_Root_from_AST(const AST *ast, const DA_Var *ast_vars);
void CNF_Clause_print(const CNF_Clause *clause);
void CNF_Root_print(const CNF_Root *root);

// TODO: Should these functions handle negated variable or the caller?
CNF_Binds CNF_Binds_make_vars(CNF_Var max_var);
CNF_Binds CNF_Binds_make_zeros(CNF_Var max_var);
CNF_Binds CNF_Binds_copy(const CNF_Binds *binds);
bool CNF_Binds_inc(CNF_Binds *binds);
void CNF_Binds_free(CNF_Binds *binds);
bool CNF_Binds_contains_var(const CNF_Binds *binds, CNF_Var var);
bool CNF_Binds_is_bound(const CNF_Binds *binds, CNF_Var var);
bool CNF_Binds_is_bound_to(const CNF_Binds *binds, CNF_Var var, bool val);
bool CNF_Binds_is_bound_true(const CNF_Binds *binds, CNF_Var var);
bool CNF_Binds_is_bound_false(const CNF_Binds *binds, CNF_Var var);
void CNF_Bind_print(CNF_Var var, const CNF_Bind *bind);
void CNF_Binds_print(const CNF_Binds *binds);

#endif // CNF_H
