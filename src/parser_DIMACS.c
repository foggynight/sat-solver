#include "parser_DIMACS.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "CNF.h"
#include "util.h"

#define LINE_MAX_DIMACS 1048576

static const char *skip_space(const char *str) {
    while (*str == ' ' || *str == '\t') { ++str; }
    return str;
}

static const char *parse_DIMACS_header(
    const char *line,
    CNF_Var *cnt_vars,
    size_t *cnt_clauses)
{
    char *tmp_end;

    if (line[0] != 'p') { return NULL; }
    line = skip_space(line + 1);

    if (strncmp(line, "cnf", 3) != 0) { return NULL; }
    line = skip_space(line + 3);

    *cnt_vars = strtoimax(line, &tmp_end, 10);
    if (*cnt_vars <= 0) { return NULL; }
    line = skip_space(tmp_end);

    *cnt_clauses = strtoumax(line, &tmp_end, 10);
    if (*cnt_clauses == 0) { return NULL; }
    line = skip_space(tmp_end);

    return line;
}

static bool parse_DIMACS_clause(
    const char *line,
    CNF_Var cnt_vars, // To check parsed variable index against max from header.
    CNF_Clause *out_clause)
{
    CNF_Clause clause = {0};
    while (true) {
        char *var_end;
        CNF_Var var = strtoimax(line, &var_end, 0);
        if (line == var_end) {
            error_msg("failed to parse variable here: %s", line);
            return false;
        }
        if (var > cnt_vars) {
            error_msg("variable out of range specified in header", line);
            return false;
        }
        line = var_end;
        if (var == 0) { break; }
        CNF_Clause_append_var(&clause, &var);
    }
    *out_clause = clause;
    return true;
}

CNF_Root *parse_DIMACS_file(FILE *file, CNF_Var *out_var_count) {
    if (file == NULL) {
        error_msg("failed to parse DIMACS file, file is NULL");
        return NULL;
    }

    char line_buffer[LINE_MAX_DIMACS];
    CNF_Root *root = CNF_Root_alloc();
    CNF_Var cnt_vars = 0;
    size_t cnt_clauses = 0;

    // Parse header from input.
    printf("Input: \"\"\"\n");
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) {
        printf("%s", line_buffer);
        const char *line = skip_space(line_buffer);
        if (line[0] == 'c' || line[0] == '\n') { continue; }  // TODO: Handle whitespace.
        if (!parse_DIMACS_header(line, &cnt_vars, &cnt_clauses)) {
            error_msg("failed to parse DIMACS header");
            goto fail;
        }
        if (cnt_vars == 0) {
            error_msg("invalid header, number of variables");
            goto fail;
        }
        if (cnt_clauses == 0) {
            error_msg("invalid header, number of clauses");
            goto fail;
        }
        break;
    }

    // Parse clauses from input.
    size_t clauses_remaining = cnt_clauses;
    while (fgets(line_buffer, sizeof(line_buffer), file) != NULL) {
        printf("%s", line_buffer);
        const char *line = skip_space(line_buffer);

        if (*line == 'p') { error_msg("found second problem header"); goto fail; }
        if (*line == 'c' || *line == '\n') { continue; }  // TODO: Handle whitespace.

        if (clauses_remaining == 0) {
            error_msg("too many clauses in input");
            goto fail;
        }
        --clauses_remaining;

        CNF_Clause clause = {0};
        if (!parse_DIMACS_clause(line, cnt_vars, &clause)) {
            error_msg("failed to parse clause");
            goto fail;
        }

        CNF_Root_append_clause(root, &clause);
    }
    printf("\"\"\"\n"); // input delimiter

    if (clauses_remaining > 0) {
        error_msg("not enough clauses in input");
        goto fail;
    }
    if (root->count != cnt_clauses) {
        error_msg(
            "number of root clauses (%zu) not equal to count specified in header (%zu)",
            root->count, cnt_clauses);
        goto fail;
    }

    *out_var_count = cnt_vars;
    return root;

fail:
    CNF_Root_free(root);
    return NULL;
}
