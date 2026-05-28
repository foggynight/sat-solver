////////////////////////////////////////////////////////////////////////////////
//
//  Boolean Expression Parser
//
//  Parser Grammar:
//    E -> T ('+' E)?
//    T -> F ('*'? T)?
//    F -> '(' E ')'
//       | '-' F
//       | [A-Za-z]
//
//  Copyright (C) 2026 Robert Coffey
//
////////////////////////////////////////////////////////////////////////////////

#ifndef PARSER_H
#define PARSER_H

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

DA_Token lex_string(const char *str);

AST *parse_tokens(DA_Token *toks, DA_char *vars);
AST *parse_string(const char *str, DA_char *vars);

#endif // PARSER_H


#ifdef PARSER_IMPL
#undef PARSER_IMPL

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#include "DA.h"

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

DA_Token lex_string(const char *str) {
    DA_Token toks = {0};
    bool error = false;

    for (const char *s = str; *s != '\0'; ++s) {
        if (isspace(*s)) { continue; }

        Token tok = {0};
        if (isalpha(*s)) { tok = (Token){ TOK_VAR, *s }; }
        else {
            switch (*s) {
            case '+': tok = (Token){ TOK_PLUS, '+' }; break;
            case '-': tok = (Token){ TOK_MINUS, '-' }; break;
            case '*': tok = (Token){ TOK_STAR, '*' }; break;
            case '(': tok = (Token){ TOK_PAREN_L, '(' }; break;
            case ')': tok = (Token){ TOK_PAREN_R, ')' }; break;
            default: error = true; tok = (Token){ TOK_ERR, *s }; break;
            }
        }

        DA_APPEND(toks, tok);
        if (error) { break; }
    }

    if (!error) { DA_APPEND(toks, ((Token){ TOK_END, '\0' })); }
    return toks;
}

AST *parse_expr(DA_Token *toks, DA_char *vars);

AST *parse_fact(DA_Token *toks, DA_char *vars) {
    if (DA_NEXT(*toks).kind == TOK_PAREN_L) {
        DA_DEQUE(*toks);
        AST *ast = parse_expr(toks, vars);
        if (DA_NEXT(*toks).kind != TOK_PAREN_R) {
            return NULL;
        }
        DA_DEQUE(*toks);
        return ast;
    }

    if (DA_NEXT(*toks).kind == TOK_MINUS) {
        AST *unary = AST_make();
        unary->kind = AST_UOP;
        unary->token = DA_NEXT(*toks);
        DA_DEQUE(*toks);

        AST *right = parse_fact(toks, vars);
        if (!right) { return NULL; }

        unary->right = right;
        return unary;
    }

    if (DA_NEXT(*toks).kind != TOK_VAR) {
        return NULL;
    }

    Token tok_var = DA_NEXT(*toks);
    DA_DEQUE(*toks);

    AST *fact = AST_make();
    fact->kind = AST_VAR;
    fact->token = tok_var;

    bool contains;
    DA_CONTAINS(*vars, tok_var.chr, &contains);
    if (!contains) DA_APPEND(*vars, tok_var.chr);

    return fact;
}

AST *parse_term(DA_Token *toks, DA_char *vars) {
    AST *fact = parse_fact(toks, vars);

    if (!fact
        || (DA_NEXT(*toks).kind != TOK_VAR
            && DA_NEXT(*toks).kind != TOK_MINUS
            && DA_NEXT(*toks).kind != TOK_STAR
            && DA_NEXT(*toks).kind != TOK_PAREN_L))
    {
        return fact;
    }

    if (DA_NEXT(*toks).kind == TOK_STAR) {
        DA_DEQUE(*toks);
    }

    AST *term = AST_make();
    term->kind = AST_BOP;
    term->token = (Token){ TOK_STAR, '*' };

    AST *right = parse_term(toks, vars);

    term->left = fact;
    term->right = right;
    return term;
}

AST *parse_expr(DA_Token *toks, DA_char *vars) {
    AST *term = parse_term(toks, vars);

    if (!term) {
        return NULL;
    }

    if (DA_NEXT(*toks).kind != TOK_PLUS) {
        return term;
    }

    AST *expr = AST_make();
    expr->kind = AST_BOP;
    expr->token = DA_NEXT(*toks);
    DA_DEQUE(*toks);

    AST *right = parse_expr(toks, vars);

    expr->left = term;
    expr->right = right;
    return expr;
}

AST *parse_tokens(DA_Token *toks, DA_char *vars) {
    AST *expr = parse_expr(toks, vars);
    if (DA_NEXT(*toks).kind != TOK_END) {
        return NULL;
    }
    return expr;
}

// TODO: This leaks tokens not stored in the AST.
AST *parse_string(const char *str, DA_char *vars) {
    DA_Token toks = lex_string(str);
    return parse_tokens(&toks, vars);
}

#endif // PARSER_IMPL
