////////////////////////////////////////////////////////////////////////////////
//
//  Parser for Infix Boolean Expressions
//
//  Parser Grammar:
//
//    E -> T ('+' E)?
//    T -> F ('*'? T)?
//    F -> '(' E ')'
//       | '-' F
//       | [A-Za-z]
//
//  e.g. "(A + B)(A + C)(B + C)"
//    => (* (+ A B) (* (+ A C) (+ B C)))
//
//  Copyright (C) 2026 Robert Coffey
//
////////////////////////////////////////////////////////////////////////////////

#ifndef PARSER_INFIX_H
#define PARSER_INFIX_H

#include <stdio.h>

#include "AST.h"

AST *parse_expr_infix(FILE *input, DA_Var *vars);

#endif // PARSER_INFIX_H
