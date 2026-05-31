#ifndef EVAL_H
#define EVAL_H

#include "AST.h"
#include "DA.h"

typedef struct {
    char var;  // variable name
    bool val;  // bound value
    bool lock; // is variable pure
} Bind;

typedef DA(Bind) DA_Bind;
typedef DA(DA_Bind) DA_DA_Bind;

void Bind_print(const Bind *bind);

DA_char vars_copy(const DA_char *vars);
void vars_free(DA_char *vars);

DA_Bind binds_zero(const DA_char *vars);  // Bindings from vars, all false.
DA_Bind binds_copy(const DA_Bind *binds);
void binds_free(DA_Bind *binds);
Bind *binds_find(DA_Bind *binds, char var);

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

#endif // EVAL_H


#ifdef EVAL_IMPL
#undef EVAL_IMPL

#include <stdbool.h>
#include <string.h>

#include "AST.h"
#include "DA.h"

static AST *binds_lookup(const DA_Bind *binds, char var) {
    for (size_t i = 0; i < binds->count; ++i) {
        Bind bind = binds->items[i];
        if (bind.var == var) {
            return bool_to_AST(bind.val);
        }
    }
    assert(0 && "unreachable");
    __builtin_unreachable();
}

DA_char vars_copy(const DA_char *vars) {
    assert(vars != NULL);
    DA_char copy = *vars;
    copy.items = malloc(vars->capacity * sizeof(char));
    assert(copy.items != NULL);
    memcpy(copy.items, vars->items, vars->count * sizeof(Bind));
    return copy;
}

void vars_free(DA_char *vars) {
    free(vars->items);
}

void Bind_print(const Bind *bind) {
    printf("(bind %c %d %d)", bind->var, bind->val, bind->lock);
}

DA_Bind binds_zero(const DA_char *vars) {
    DA_Bind binds = {0};
    for (size_t i = 0; i < vars->count; ++i) {
        DA_APPEND(binds, ((Bind){ vars->items[i], false, false }));
    }
    return binds;
}

DA_Bind binds_copy(const DA_Bind *binds) {
    DA_Bind copy = *binds;
    copy.items = malloc(binds->capacity * sizeof(Bind));
    assert(copy.items != NULL);
    memcpy(copy.items, binds->items, binds->count * sizeof(Bind));
    return copy;
}

void binds_free(DA_Bind *binds) {
    free(binds->items);
}

Bind *binds_find(DA_Bind *binds, char var) {
    for (size_t i = 0; i < binds->count; ++i) {
        if (binds->items[i].var == var) {
            return &(binds->items[i]);
        }
    }
    return NULL;
}

bool binds_inc(DA_Bind *binds) {
    bool carry = true;
    for (size_t i = 0; i < binds->count; ++i) {
        if (binds->items[i].lock == true) { continue; }
        const bool next_val = (binds->items[i].val != carry);
        carry = (binds->items[i].val && carry);
        binds->items[i].val = next_val;
        if (!carry) { break; }
    }
    return carry;
}

void binds_print(const DA_Bind *binds) {
    bool first = true;
    for (size_t i = 0; i < binds->count; ++i) {
        if (first) {
            first = false;
        } else {
            putchar(' ');
        }
        Bind_print(&binds->items[i]);
    }
}

AST *eval_ast_binds(const AST *ast, const DA_Bind *binds) {
    switch (ast->kind) {
    case AST_TRUE: return AST_true();
    case AST_FALSE: return AST_false();

    case AST_VAR:
        return binds_lookup(binds, ast->token.chr);

    case AST_UOP:
        if (ast->token.chr != '-') { return NULL; }
        return AST_not(eval_ast_binds(ast->right, binds));

    case AST_BOP: {
        const AST *left_val = eval_ast_binds(ast->left, binds);
        const AST *right_val = eval_ast_binds(ast->right, binds);
        switch (ast->token.chr) {
        case '*':
            return AST_and(left_val, right_val);
        case '+':
            return AST_or(left_val, right_val);
        default:
            return NULL;
        }
        assert(0 && "unreachable");
        __builtin_unreachable();
    }

    default:
        return NULL;
    }

    assert(0 && "unreachable");
    __builtin_unreachable();
}

#endif // EVAL_IMPL
