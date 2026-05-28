#ifndef EVAL_H
#define EVAL_H

#include "AST.h"
#include "DA.h"

typedef struct {
    char var;
    bool val;
} Bind;

typedef DA(Bind) DA_Bind;

AST *eval_ast_binds(AST *ast, DA_Bind *binds);

#endif // EVAL_H


#ifdef EVAL_IMPL
#undef EVAL_IMPL

#include <stdbool.h>

#include "AST.h"
#include "DA.h"

static AST *binds_lookup(DA_Bind *binds, char var) {
    for (size_t i = 0; i < binds->count; ++i) {
        Bind bind = binds->items[i];
        if (bind.var == var) {
            return bool_to_AST(bind.val);
        }
    }
    assert(0 && "unreachable");
    __builtin_unreachable();
}

AST *eval_ast_binds(AST *ast, DA_Bind *binds) {
    switch (ast->kind) {
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

    case AST_TRUE: [[fallthrough]];
    case AST_FALSE:
        return ast;

    default:
        return NULL;
    }

    assert(0 && "unreachable");
    __builtin_unreachable();
}

#endif // EVAL_IMPL
