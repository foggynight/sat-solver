#ifndef AST_H
#define AST_H

#include "DA.h"

typedef enum TokenKind {
    TOK_ERR,
    TOK_END,

    TOK_VAR,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,

    TOK_PAREN_L,
    TOK_PAREN_R,
} TokenKind;

typedef struct Token {
    TokenKind kind;
    char chr;
} Token;

typedef enum ASTKind {
    AST_ERR,

    AST_TRUE,
    AST_FALSE,

    AST_VAR,
    AST_UOP,
    AST_BOP,
} ASTKind;

typedef struct AST {
    ASTKind kind;
    Token token;
    struct AST *left;
    struct AST *right;
} AST;

typedef DA(Token) DA_Token;

AST *AST_make(void);
void AST_free(AST *ast);

AST *AST_true(void);
AST *AST_false(void);

bool AST_is_bool(const AST *ast);
bool AST_to_bool(const AST *ast);
AST *bool_to_AST(bool val);

AST *AST_not(const AST *ast);
AST *AST_and(const AST *ast1, const AST *ast2);
AST *AST_or(const AST *ast1, const AST *ast2);

void AST_print(const AST *ast);

#endif // AST_H


#ifdef AST_IMPL
#undef AST_IMPL

#include <assert.h>
#include <stdlib.h>

AST *AST_make(void) {
    AST *ast = calloc(1, sizeof(AST));
    assert(ast != NULL);
    return ast;
}

void AST_free(AST *ast) {
    if (ast->left != NULL) { AST_free(ast->left); }
    if (ast->right != NULL) { AST_free(ast->right); }
    free(ast);
}

AST *AST_true(void) {
    AST *ast = AST_make();
    ast->kind = AST_TRUE;
    return ast;
}

AST *AST_false(void) {
    AST *ast = AST_make();
    ast->kind = AST_FALSE;
    return ast;
}

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

AST *AST_not(const AST *ast) {
    if (!ast) { return NULL; }
    if (!AST_is_bool(ast)) { return NULL; }
    return (ast->kind == AST_TRUE) ? AST_false() : AST_true();
}

AST *AST_and(const AST *ast1, const AST *ast2) {
    if (!ast1 || !ast2) { return NULL; }
    if (!AST_is_bool(ast1) || !AST_is_bool(ast2)) { return NULL; }
    return (ast1->kind == AST_TRUE && ast2->kind == AST_TRUE)
        ? AST_true() : AST_false();
}

AST *AST_or(const AST *ast1, const AST *ast2) {
    if (!ast1 || !ast2) { return NULL; }
    if (!AST_is_bool(ast1) || !AST_is_bool(ast2)) { return NULL; }
    return (ast1->kind == AST_TRUE || ast2->kind == AST_TRUE)
        ? AST_true() : AST_false();
}

void AST_print(const AST *ast) {
    if (!ast) {
        printf("NULL");
    } else if (!(ast->left || ast->right)) {
        printf("%c", ast->token.chr);
    } else {
        putchar('(');
        printf("%c", ast->token.chr);
        if (ast->left != NULL) { putchar(' '); AST_print(ast->left); }
        if (ast->right != NULL) { putchar(' '); AST_print(ast->right); }
        putchar(')');
    }
}

#endif // AST_IMPL
