#include "AST.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

Token Token_plus(void) { return (Token){ TOK_PLUS, "+" }; }
Token Token_minus(void) { return (Token){ TOK_MINUS, "-" }; }
Token Token_star(void) { return (Token){ TOK_STAR, "*" }; }

static AST_list *AST_list_make(void) {
    AST_list *list = calloc(1, sizeof(AST_list));
    assert(list != NULL);
    return list;
}

static AST_list *AST_list_append(AST_list *list, AST *ast) {
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
    free(ast);
}

AST *AST_true(void) { return &ast_true; }
AST *AST_false(void) { return &ast_false; }
AST *AST_null(void) { return &ast_null; }

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

void AST_print(const AST *ast) {
    if (!ast) {
        printf("NULL");
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
