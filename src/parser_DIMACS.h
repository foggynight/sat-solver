////////////////////////////////////////////////////////////////////////////////
//
//  Parser for DIMACS CNF File Format
//
//  Parser Grammar:
//
//    FILE -> COMMENT* HEADER LINE*
//    LINE -> COMMENT | CLAUSE
//
//    COMMENT -> 'c' .* '\n'
//    HEADER  -> 'p' SPACE "cnf" SPACE {# vars} SPACE {# clauses} '\n'
//    CLAUSE  -> SPACE ('-'? VARIABLE SPACE){# vars} '0' '\n'
//
//    SPACE -> [ \t]*
//
//  Copyright (C) 2026 Robert Coffey
//
////////////////////////////////////////////////////////////////////////////////

#ifndef PARSER_DIMACS_H
#define PARSER_DIMACS_H

#include <stdio.h>

#include "CNF.h"

CNF_Root *parse_DIMACS_file(FILE *file, CNF_Var *out_var_count);

#endif // PARSER_DIMACS_H
