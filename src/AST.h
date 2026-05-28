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

typedef DA(char) DA_char;
typedef DA(Token) DA_Token;

void AST_print(const AST *ast);
void AST_free(AST *ast);

#endif // AST_H


#ifdef AST_IMPL
#undef AST_IMPL

#include <assert.h>
#include <stdlib.h>

static AST *AST_make(void) {
    AST *ast = calloc(1, sizeof(AST));
    assert(ast != NULL);
    return ast;
}

void AST_free(AST *ast) {
    if (ast->left != NULL) { AST_free(ast->left); }
    if (ast->right != NULL) { AST_free(ast->right); }
    free(ast);
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
