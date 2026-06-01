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

AST *parse_DIMACS_file(FILE *file, DA_char *vars);

#endif // PARSER_DIMACS_H


#ifdef PARSER_DIMACS_IMPL
#undef PARSER_DIMACS_IMPL

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "AST.h"

#define DIMACS_LINE_MAX 1048576

typedef const char * DIMACS_Variable;
typedef DA(DIMACS_Variable) DIMACS_Clause;
typedef DA(DIMACS_Clause) DA_DIMACS_Clause;

static const char *skip_space(const char *str) {
    while (*str == ' ' || *str == '\t') { ++str; }
    return str;
}

static const char *next_space(const char *str) {
    while (*str != ' ' && *str != '\t' && *str != '\n') { ++str; }
    return str;
}

static char *string_slice(const char *str, size_t i_start, size_t i_end) {
    char *copy = malloc(i_end - i_start);
    if (!copy) { return NULL; }
    for (size_t i = i_start; i < i_end; ++i) {
        copy[i] = str[i];
    }
    return copy;
}

const char *parse_DIMACS_header(
    const char *line,
    uint64_t *cnt_vars,
    uint64_t *cnt_clauses)
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

AST *parse_DIMACS_file(FILE *file, DA_char *vars) {
    if (file == NULL) {
        fprintf(stderr, "error: failed to parse DIMACS file, file is NULL");
        return NULL;
    }

    char line_buffer[DIMACS_LINE_MAX];
    DA_DIMACS_Clause clauses = {0};
    uint64_t cnt_vars = 0, cnt_clauses = 0;

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

        // TODO: temp
        printf("# vars = %ld, # clauses = %ld\n", cnt_vars, cnt_clauses);

        break;
    }

    // Parse clauses from input.
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) {
        const char *line = skip_space(line_buffer);

        // Skip comments and empty line.
        if (line[0] == 'c' || line[0] == '\n') { continue; }

        if (cnt_clauses == 0) {
            fprintf(stderr, "error: too many clauses in input\n");
            return NULL;
        }
        --cnt_clauses;

        DIMACS_Clause clause = {0};
        for (uint64_t i = 0; i < cnt_vars; ++i) {
            const char *var_end = next_space(line);
            if (line == var_end) {
                fprintf(stderr, "error: failed to parse clause, not enough variables\n");
                return NULL;
            }
            line = skip_space(var_end);
            DA_APPEND(clause, string_slice(line, 0, var_end - line));
        }
        DA_APPEND(clauses, clause);
    }
    if (cnt_clauses > 0) {
        fprintf(stderr, "error: not enough clauses in input\n");
        return NULL;
    }

    // Build AST from clauses.
    AST *ast = NULL;

    DA_FREE(clauses);
    return ast;
}

#endif // PARSER_DIMACS_IMPL
