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
typedef DA_Bind Solution;
typedef DA(Solution) DA_Solution;

void Bind_print(const Bind *bind);

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

DA_Solution solve_brute_force(
    const AST *ast,
    const DA_Var *vars,
    bool first_solution);

DA_Solution solve_DPLL(
    const AST *ast_original,
    const DA_Var *vars_original,
    bool first_solution);

#endif // EVAL_H
