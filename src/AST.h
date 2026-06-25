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

// TODO: TEMP
#define AST_Var Var
#define DA_AST_Var DA_Var

typedef char * Var;

typedef struct {
    Var var;   // variable name
    bool val;  // bound value
    bool pure; // is variable pure
} Bind;

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
typedef DA(Bind) DA_Bind;
typedef DA(Token) DA_Token;

DA_Var vars_copy(const DA_Var *vars);
void vars_free(DA_Var *vars);
bool vars_find(const DA_Var *vars, Var targ, size_t *out_index);

void Bind_print(const Bind *bind);

DA_Bind binds_zero(const DA_Var *vars);  // Bindings from vars, all false.
DA_Bind binds_copy(const DA_Bind *binds);
void binds_free(DA_Bind *binds);
Bind *binds_find(DA_Bind *binds, Var var);

// Increment variable bindings, like binary increment with first binding
// corresponding to least significant digit.
//
// Skips pure variables as if they don't exist.
//
// Returns true when binds was modified, else false.
bool binds_inc(DA_Bind *binds);

void binds_print(const DA_Bind *binds);

Token Token_plus(void);
Token Token_minus(void);
Token Token_star(void);

AST_list *AST_list_make(void);
AST_list *AST_list_append(AST_list *list, AST *ast);
void AST_list_free(AST_list *ast_list);

AST *AST_make(void);
AST *AST_append(AST *parent, AST *child);
AST *AST_copy(const AST *ast);
void AST_free(AST *ast);

AST *AST_true(void);
AST *AST_false(void);
AST *AST_null(void);

bool AST_is_bool(const AST *ast);
bool AST_to_bool(const AST *ast);
AST *bool_to_AST(bool val);

bool AST_is_var(const AST *ast);
bool AST_is_op(const AST *ast);
bool AST_is_not(const AST *ast);
bool AST_is_and(const AST *ast);
bool AST_is_or(const AST *ast);
bool AST_is_lit(const AST *ast);
bool AST_is_CNF(const AST *ast);

bool AST_has_single_child(const AST *ast);

bool AST_eq_var(const AST *ast, Var var);
bool AST_eq_negvar(const AST *ast, Var var);

AST *AST_make_var(Var var);
AST *AST_make_not(AST *ast);
AST *AST_make_and(void);
AST *AST_make_or(void);

AST *AST_eval_not(const AST *ast);
AST *AST_eval_and(const AST *ast1, const AST *ast2);
AST *AST_eval_or(const AST *ast1, const AST *ast2);

// Evaluate AST to true/false given variable bindings.
AST *AST_eval_binds(const AST *ast, const DA_Bind *binds);

void AST_print(const AST *ast);

#endif // AST_H
