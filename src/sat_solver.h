////////////////////////////////////////////////////////////////////////////////
//
//  SAT Solving Algorithms
//
//  Copyright (C) 2026 Robert Coffey
//
////////////////////////////////////////////////////////////////////////////////

#ifndef SAT_SOLVER_H
#define SAT_SOLVER_H

#include "AST.h"
#include "DA.h"

typedef DA_Bind Solution;
typedef DA(Solution) DA_Solution;

DA_Solution solve_brute_force(
    const AST *ast,
    const DA_Var *vars,
    bool first_solution);

DA_Solution solve_DPLL(
    const AST *ast_original,
    const DA_Var *vars_original,
    bool first_solution);

#endif // SAT_SOLVER_H
