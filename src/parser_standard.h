////////////////////////////////////////////////////////////////////////////////
//
//  Parser for "Standard" Boolean Expressions
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

#ifndef PARSER_STANDARD_H
#define PARSER_STANDARD_H

#include <stdio.h>

#include "AST.h"

AST *parse_standard_expr(FILE *input, DA_Var *vars);

#endif // PARSER_STANDARD_H
