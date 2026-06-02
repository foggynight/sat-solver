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

#include "AST.h"

AST *parse_DIMACS_file(FILE *file, DA_Var *vars);

#endif // PARSER_DIMACS_H


#ifdef PARSER_DIMACS_IMPL
#undef PARSER_DIMACS_IMPL

#include <stdbool.h>
#include <stdio.h>

#include "AST.h"
#include "util.h"

#define DIMACS_LINE_MAX 1048576

static const char *skip_space(const char *str) {
    while (*str == ' ' || *str == '\t') { ++str; }
    return str;
}

static const char *next_blank(const char *str) {
    while (*str != ' ' && *str != '\t' && *str != '\n' && *str != '\0') {
        ++str;
    }
    return str;
}

static const char *parse_DIMACS_header(
    const char *line,
    size_t *cnt_vars,
    size_t *cnt_clauses)
{
    char *tmp_end;

    if (line[0] != 'p') { return NULL; }
    line = skip_space(line + 1);

    if (strncmp(line, "cnf", 3) != 0) { return NULL; }
    line = skip_space(line + 3);

    *cnt_vars = strtoul(line, &tmp_end, 10);
    if (*cnt_vars == 0) { return NULL; }
    line = skip_space(tmp_end);

    *cnt_clauses = strtoll(line, &tmp_end, 10);
    if (*cnt_clauses == 0) { return NULL; }
    line = skip_space(tmp_end);

    return line;
}

static const char *parse_DIMACS_clause(
    const char *line,
    DA_Var *vars,
    AST **out_clause)
{
    AST *clause = NULL;

    while (true) {
        const char *term_end = next_blank(line);

        const bool negated = (*line == '-');
        if (negated) { ++line; }

        const size_t term_len = term_end - line;
        const Var var = string_slice(line, 0, term_len);

        if (strcmp(var, "0") == 0) {
            if (negated) {
                fprintf(
                    stderr,
                    "error: invalid term, negated zero\nline: %s\n",
                    line);
                return NULL;
            }
            break;
        }

        bool contains;
        DA_CONTAINS_STR(*vars, var, &contains);
        if (!contains) { DA_APPEND(*vars, var); }

        AST * const base_term = AST_make_var(var);
        AST * const term = negated ? AST_make_not(base_term) : base_term;
        clause = (clause == NULL) ? term : AST_make_or(clause, term);

        line = skip_space(term_end);
    }

    *out_clause = clause;
    return line;
}

AST *parse_DIMACS_file(FILE *file, DA_Var *vars) {
    if (file == NULL) {
        fprintf(stderr, "error: failed to parse DIMACS file, file is NULL");
        return NULL;
    }

    char line_buffer[DIMACS_LINE_MAX];
    DA_AST_ptr clauses = {0};
    size_t cnt_vars = 0, cnt_clauses = 0;

    // Parse header from input.
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) {
        const char *line = skip_space(line_buffer);

        // Skip comments and empty line.
        if (line[0] == 'c' || line[0] == '\n') { continue; }

        if (!parse_DIMACS_header(line, &cnt_vars, &cnt_clauses)) {
            fprintf(stderr, "error: failed to parse DIMACS header\n");
            return NULL;
        }
        if (cnt_vars == 0) {
            fprintf(stderr, "error: invalid header, number of variables\n");
            return NULL;
        }
        if (cnt_clauses == 0) {
            fprintf(stderr, "error: invalid header, number of clauses\n");
            return NULL;
        }

        break;
    }

    // Parse clauses from input.
    size_t clauses_remaining = cnt_clauses;
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) {
        const char *line = skip_space(line_buffer);

        // Skip comments and empty line.
        if (line[0] == 'c' || line[0] == '\n') { continue; }

        if (clauses_remaining == 0) {
            fprintf(stderr, "error: too many clauses in input\n");
            return NULL;
        }
        --clauses_remaining;

        AST *clause = NULL;
        line = parse_DIMACS_clause(line, vars, &clause);
        DA_APPEND(clauses, clause);
    }

    assert(clauses.count == cnt_clauses);
    if (clauses_remaining > 0) {
        fprintf(stderr, "error: not enough clauses in input\n");
        return NULL;
    }
    if (vars->count > cnt_vars) {
        fprintf(stderr, "error: too many variables in input\n");
        return NULL;
    }

    // Build AST from clauses.
    AST *ast = NULL;
    for (size_t i = 0; i < clauses.count; ++i) {
        AST *clause = clauses.items[i];
        if (!ast) { ast = clause; }
        else { ast = AST_make_and(ast, clause); }
    }

    free(clauses.items);
    return ast;
}

#endif // PARSER_DIMACS_IMPL
