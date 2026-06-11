////////////////////////////////////////////////////////////////////////////////
//
//  Abstract Syntax Tree for Boolean Expressions
//
//  Copyright (C) 2026 Robert Coffey
//
////////////////////////////////////////////////////////////////////////////////

#ifndef AST_H
#define AST_H

#include "DA.h"

typedef char * Var;

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
    Var var;
} Token;

typedef enum ASTKind {
    AST_ERR,
    AST_NULL,

    AST_TRUE,
    AST_FALSE,

    AST_VAR,
    AST_OP,
} ASTKind;

typedef struct AST {
    ASTKind kind;
    Token token;
    struct AST_list *children;
} AST;

typedef struct AST_list {
    AST *ast;
    struct AST_list *next;
} AST_list;

typedef DA(Var) DA_Var;
typedef DA(Token) DA_Token;

DA_Var vars_copy(const DA_Var *vars);
void vars_free(DA_Var *vars);

Token Token_plus(void);
Token Token_minus(void);
Token Token_star(void);

AST *AST_make(void);
AST *AST_append(AST *parent, AST *child);
AST *AST_copy(const AST *ast);
void AST_free(AST *ast);

AST *AST_true(void);
AST *AST_false(void);
AST *AST_null(void);

AST *AST_make_var(Var var);
AST *AST_make_not(AST *ast);
AST *AST_make_and(void);
AST *AST_make_or(void);

bool AST_is_bool(const AST *ast);
bool AST_to_bool(const AST *ast);
AST *bool_to_AST(bool val);

AST *AST_eval_not(const AST *ast);
AST *AST_eval_and(const AST *ast1, const AST *ast2);
AST *AST_eval_or(const AST *ast1, const AST *ast2);

void AST_print(const AST *ast);

#endif // AST_H
