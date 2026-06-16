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
#include "CNF.h"
#include "DA.h"

//typedef struct {
//    CNF_Var var;
//    CNF_Binds bind;
//} Solution;
typedef CNF_Binds Solution;
typedef DA(Solution) DA_Solution;

bool solve_brute_force(
    const CNF_Root *cnf_root,
    CNF_Var max_var,
    bool first_solution,
    DA_Solution *out_solutions);

bool solve_DPLL(
    const CNF_Root *cnf_root,
    const CNF_Var max_var,
    bool first_solution,
    DA_Solution *out_solutions);

#endif // SAT_SOLVER_H
