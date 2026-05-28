////////////////////////////////////////////////////////////////////////////////
//
//  Boolean Expression Parser
//
//  Parser Grammar:
//    E -> '(' E ')'
//       | T ([+] E)?
//    T -> P ([*]? T)?
//    P -> [A-Za-z]
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

typedef DA(Token) DA_Token;

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

void AST_print(const AST *ast);
void AST_free(AST *ast);

DA_Token lex_string(const char *str);

AST *parse_tokens(DA_Token *toks);
AST *parse_string(const char *str);

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

// TODO: Make left-associative.
// TODO: Add operator precedence.
// TODO: Add unary operators.
AST *parse_expr(DA_Token *toks) {
    if (DA_NEXT(*toks).kind == TOK_END
        || DA_NEXT(*toks).kind == TOK_ERR)
    {
        AST *err = AST_make();
        err->kind = AST_ERR;
        return err;
    }

    if (DA_NEXT(*toks).kind == TOK_PAREN_L) {
        DA_DEQUE(*toks);
        AST *ast = parse_expr(toks);
        if (DA_NEXT(*toks).kind != TOK_PAREN_R) { return NULL; }
        DA_DEQUE(*toks);
        return ast;
    }

    AST *ast = NULL;

    if (DA_NEXT(*toks).kind == TOK_VAR) {
        ast = AST_make();
        ast->kind = AST_VAR;
        ast->token = DA_NEXT(*toks);
        DA_DEQUE(*toks);

        switch (DA_NEXT(*toks).kind) {
        case TOK_END: {
            ;  // Done, NOP.
        } break;

        case TOK_PAREN_R:
            ;  // Done, NOP.
            break;

        case TOK_VAR: {
            AST *left = ast;

            ast = AST_make();
            ast->kind = AST_BOP;
            ast->token = (Token){ TOK_STAR, '*' };

            AST *right = parse_expr(toks);

            ast->left = left;
            ast->right = right;
        } break;

        case TOK_PLUS: [[fallthrough]];
        case TOK_MINUS: [[fallthrough]];
        case TOK_STAR: {
            AST *left = ast;

            ast = AST_make();
            ast->kind = AST_BOP;
            ast->token = DA_NEXT(*toks);
            DA_DEQUE(*toks);

            AST *right = parse_expr(toks);

            ast->left = left;
            ast->right = right;
        } break;

        default: {
            AST_free(ast);
            ast = NULL;
        } break;
        }
    }

    return ast;
}

// Parse a single boolean expression.
AST *parse_tokens(DA_Token *toks) {
    AST *expr = parse_expr(toks);
    return expr;
}

// TODO: This leaks tokens not stored in the AST.
AST *parse_string(const char *str) {
    DA_Token toks = lex_string(str);
    return parse_tokens(&toks);
}

#endif // PARSER_IMPL
