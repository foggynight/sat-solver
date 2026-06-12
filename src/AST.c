#include "AST.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DA.h"
#include "util.h"

AST ast_true = { AST_TRUE, {0}, NULL };
AST ast_false = { AST_FALSE, {0}, NULL };
AST ast_null = { AST_NULL, {0}, NULL };

DA_Var vars_copy(const DA_Var *vars) {
    assert(vars != NULL);
    DA_Var copy = *vars;
    copy.items = malloc(vars->capacity * sizeof(Var));
    assert(copy.items != NULL);
    memcpy(copy.items, vars->items, vars->count * sizeof(Var));
    return copy;
}

void vars_free(DA_Var *vars) { free(vars->items); }

static AST *binds_lookup(const DA_Bind *binds, Var var) {
    for (size_t i = 0; i < binds->count; ++i) {
        Bind bind = binds->items[i];
        if (strcmp(bind.var, var) == 0) {
            return bool_to_AST(bind.val);
        }
    }
    assert(0 && "unreachable");
    __builtin_unreachable();
}

void Bind_print(const Bind *bind) {
    const char *name = bind->pure ? "pure" : "bind";
    printf("(%s %s %d)", name, bind->var, bind->val);
}

DA_Bind binds_zero(const DA_Var *vars) {
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

Bind *binds_find(DA_Bind *binds, Var var) {
    for (size_t i = 0; i < binds->count; ++i) {
        if (strcmp(binds->items[i].var, var) == 0) {
            return &(binds->items[i]);
        }
    }
    return NULL;
}

bool binds_inc(DA_Bind *binds) {
    bool carry = true;
    for (size_t i = 0; i < binds->count; ++i) {
        if (binds->items[i].pure == true) { continue; }
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

Token Token_plus(void) { return (Token){ TOK_PLUS, "+" }; }
Token Token_minus(void) { return (Token){ TOK_MINUS, "-" }; }
Token Token_star(void) { return (Token){ TOK_STAR, "*" }; }

AST_list *AST_list_make(void) {
    AST_list *ast_list = calloc(1, sizeof(AST_list));
    assert(ast_list != NULL);
    return ast_list;
}

AST_list *AST_list_append(AST_list *list, AST *ast) {
    if (list == NULL) {
        list = AST_list_make();
        list->ast = ast;
        return list;
    }

    AST_list * const head = list;
    for (;
         list->next != NULL;
         list = list->next)
    {}
    list->next = AST_list_make();
    list->next->ast = ast;
    return head;
}

void AST_list_free(AST_list *ast_list) {
    AST_list *prev;
    while (ast_list != NULL) {
        prev = ast_list;
        ast_list = ast_list->next;
        free(prev);
    }
}

AST *AST_make(void) {
    AST *ast = calloc(1, sizeof(AST));
    assert(ast != NULL);
    return ast;
}

AST *AST_append(AST *parent, AST *child) {
    parent->children = AST_list_append(parent->children, child);
    return parent;
}

AST *AST_copy(const AST *ast) {
    if (ast == NULL) { return NULL; }
    AST *copy = AST_make();
    copy->kind = ast->kind;
    copy->token = ast->token; // TODO: Should copy/free token, for now let it leak.
    for (AST_list *walk = ast->children; walk != NULL; walk = walk->next) {
        AST_append(copy, AST_copy(walk->ast));
    }
    return copy;
}

void AST_free(AST *ast) {
    if (ast == &ast_true || ast == &ast_false) { return; }
    for (AST_list *walk = ast->children;
         walk != NULL;
         walk = walk->next)
    {
        AST_free(walk->ast);
    }
    AST_list_free(ast->children);
    free(ast);
}

AST *AST_true(void) { return &ast_true; }
AST *AST_false(void) { return &ast_false; }
AST *AST_null(void) { return &ast_null; }

bool AST_is_bool(const AST *ast) {
    return ast->kind == AST_TRUE || ast->kind == AST_FALSE;
}

bool AST_to_bool(const AST *ast) {
    assert(AST_is_bool(ast));
    return (ast->kind == AST_TRUE) ? true : false;
}

AST *bool_to_AST(bool val) {
    return val ? AST_true() : AST_false();
}

bool AST_is_var(const AST *ast) {
    return ast->kind == AST_VAR;
}

bool AST_is_op(const AST *ast) {
    return ast->kind == AST_OP;
}

bool AST_is_not(const AST *ast) {
    return ast->kind == AST_OP && ast->token.kind == TOK_MINUS;
}

bool AST_is_and(const AST *ast) {
    return ast->kind == AST_OP && ast->token.kind == TOK_STAR;
}

bool AST_is_or(const AST *ast) {
    return ast->kind == AST_OP && ast->token.kind == TOK_PLUS;
}

bool AST_has_single_child(const AST *ast) {
    return ast->children != NULL && ast->children->next == NULL;
}

static bool AST_is_negvar(const AST *ast) {
    return AST_is_not(ast)
        && AST_is_var(ast->children->ast)
        && AST_has_single_child(ast);
}

static bool AST_is_disj_vars_or_negs(const AST *ast) {
    if (AST_is_var(ast)) { return true; }
    else if (!AST_is_or(ast)) { return false; }

    for (AST_list *walk = ast->children; walk != NULL; walk = walk->next) {
        if (!AST_is_var(walk->ast) && !AST_is_negvar(walk->ast)) {
            return false;
        }
    }
    return true;
}

// TODO: There's some cases which this detects as not CNF but might count.
// e.g. (A B)(A + B)(B + C), (A + B) + (B + C)
bool AST_is_CNF(const AST *ast) {
    if (AST_is_var(ast)) { return true; }
    else if (!AST_is_and(ast)) { return false; }

    for (AST_list *walk = ast->children; walk != NULL; walk = walk->next) {
        if (!AST_is_disj_vars_or_negs(walk->ast)) {
            return false;
        }
    }
    return true;
}

bool AST_eq_var(const AST *ast, Var var) {
    return AST_is_var(ast) && strcmp(ast->token.var, var) == 0;
}

bool AST_eq_negvar(const AST *ast, Var var) {
    if (!AST_is_not(ast)) { return false; }
    return AST_eq_var(ast->children->ast, var);
}

AST *AST_make_var(Var var) {
    AST *ast = AST_make();
    ast->kind = AST_VAR;
    ast->token = (Token){ TOK_VAR, var };
    return ast;
}

AST *AST_make_not(AST *ast) {
    AST *new_ast = AST_make();
    new_ast->kind = AST_OP;
    new_ast->token = Token_minus();
    AST_append(new_ast, ast);
    return new_ast;
}

static AST *AST_make_op(const Token tok) {
    AST *new_ast = AST_make();
    new_ast->kind = AST_OP;
    new_ast->token = tok;
    return new_ast;
}

AST *AST_make_and(void) { return AST_make_op(Token_star()); }
AST *AST_make_or(void) { return AST_make_op(Token_plus()); }

AST *AST_eval_not(const AST *ast) {
    if (!ast) { return NULL; }
    if (!AST_is_bool(ast)) { return NULL; }
    return (ast->kind == AST_TRUE) ? AST_false() : AST_true();
}

AST *AST_eval_and(const AST *ast1, const AST *ast2) {
    if (!ast1 || !ast2) { return NULL; }
    if (!AST_is_bool(ast1) || !AST_is_bool(ast2)) { return NULL; }
    return (ast1->kind == AST_TRUE && ast2->kind == AST_TRUE)
        ? AST_true() : AST_false();
}

AST *AST_eval_or(const AST *ast1, const AST *ast2) {
    if (!ast1 || !ast2) { return NULL; }
    if (!AST_is_bool(ast1) || !AST_is_bool(ast2)) { return NULL; }
    return (ast1->kind == AST_TRUE || ast2->kind == AST_TRUE)
        ? AST_true() : AST_false();
}

AST *AST_eval_binds(const AST *ast, const DA_Bind *binds) {
    switch (ast->kind) {
    case AST_TRUE: return AST_true();
    case AST_FALSE: return AST_false();

    case AST_VAR: return binds_lookup(binds, ast->token.var);

    case AST_OP:
        assert(ast->children != NULL); // Operators require children.
        AST *evaluated_ast;
        AST_list *child_list;

        // TODO: Remove code duplication in TOK_STAR and TOK_PLUS.
        switch (ast->token.kind) {
        case TOK_MINUS:
            assert(ast->children->next == NULL); // Assert single child.
            return AST_eval_not(AST_eval_binds(ast->children->ast, binds));

        case TOK_STAR:
            evaluated_ast = NULL;
            child_list = ast->children;
            do {
                AST *evaluated_child = AST_eval_binds(child_list->ast, binds);
                if (evaluated_ast == NULL) {
                    evaluated_ast = evaluated_child;
                } else {
                    evaluated_ast = AST_eval_and(evaluated_ast, evaluated_child);
                }
            } while ((child_list = child_list->next) != NULL);
            return evaluated_ast;

        case TOK_PLUS:
            evaluated_ast = NULL;
            child_list = ast->children;
            do {
                AST *evaluated_child = AST_eval_binds(child_list->ast, binds);
                if (evaluated_ast == NULL) {
                    evaluated_ast = evaluated_child;
                } else {
                    evaluated_ast = AST_eval_or(evaluated_ast, evaluated_child);
                }
            } while ((child_list = child_list->next) != NULL);
            return evaluated_ast;

        default: UNREACHABLE();
        }
        break;

    default: return NULL;
    }

    UNREACHABLE();
}

void AST_print(const AST *ast) {
    if (!ast) {
        printf("NULL");
    } else if (ast->kind == AST_TRUE) {
        printf("TRUE");
    } else if (ast->kind == AST_FALSE) {
        printf("FALSE");
    } else if (ast->kind == AST_VAR) {
        printf("%s", ast->token.var);
    } else if (ast->kind == AST_OP) {
        putchar('(');
        printf("%s", ast->token.var);
        for (AST_list *walk = ast->children;
             walk != NULL;
             walk = walk->next)
        {
            putchar(' ');
            AST_print(walk->ast);
        }
        putchar(')');
    } else {
        assert(0 && "unreachable");
        __builtin_unreachable();
    }
}
