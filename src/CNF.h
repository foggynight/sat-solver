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

CNF_Root *CNF_Root_alloc(void);
void CNF_Root_free(CNF_Root *root);

CNF_Root *CNF_Root_from_AST(const AST *ast, const DA_Var *ast_vars);

void CNF_Root_print(CNF_Root *root);

#endif // CNF_H
