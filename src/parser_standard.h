////////////////////////////////////////////////////////////////////////////////
//
//  Parser for "Standard" Boolean Expressions
//
//  Parser Grammar:
//
//    E -> T ('+' E)?
//    T -> F ('*'? T)?
//    F -> '(' E ')'
//       | '-' F
//       | [A-Za-z]
//
//  e.g. "(A + B)(A + C)(B + C)"
//    => (* (+ A B) (* (+ A C) (+ B C)))
//
//  Copyright (C) 2026 Robert Coffey
//
////////////////////////////////////////////////////////////////////////////////

#ifndef PARSER_STANDARD_H
#define PARSER_STANDARD_H

#include "AST.h"

AST *parse_standard_expr(FILE *input, DA_Var *vars);

#endif // PARSER_STANDARD_H


#ifdef PARSER_STANDARD_IMPL
#undef PARSER_STANDARD_IMPL

#include <ctype.h>

#include "AST.h"
#include "DA.h"
#include "util.h"

#define LINE_MAX_STANDARD 1048576

static DA_Token lex_string(const char *str) {
    DA_Token toks = {0};
    bool error = false;

    for (const char *s = str; *s != '\0'; ++s) {
        if (isspace(*s)) { continue; }

        Token tok = {0};
        if (isalnum(*s)) {
            const char *end = s;
            while (isalnum(*end)) { ++end; }
            tok = (Token){ TOK_VAR, string_slice(s, 0, end - s) };
            s = end - 1;
        } else {
            switch (*s) {
            case '+': tok = (Token){ TOK_PLUS, "+" }; break;
            case '-': tok = (Token){ TOK_MINUS, "-" }; break;
            case '*': tok = (Token){ TOK_STAR, "*" }; break;
            case '(': tok = (Token){ TOK_PAREN_L, "(" }; break;
            case ')': tok = (Token){ TOK_PAREN_R, ")" }; break;
            default: error = true; tok = (Token){ TOK_ERR, NULL }; break;
            }
        }

        DA_APPEND(toks, tok);
        if (error) { break; }
    }

    if (!error) { DA_APPEND(toks, ((Token){ TOK_END, NULL })); }
    return toks;
}

static AST *parse_expr(DA_Token *toks, DA_Var *vars);

static AST *parse_fact(DA_Token *toks, DA_Var *vars) {
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
    DA_CONTAINS_STR(*vars, tok_var.var, &contains);
    if (!contains) DA_APPEND(*vars, tok_var.var);

    return fact;
}

static AST *parse_term(DA_Token *toks, DA_Var *vars) {
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
    term->token = (Token){ TOK_STAR, "*" };

    AST *right = parse_term(toks, vars);

    term->left = fact;
    term->right = right;
    return term;
}

static AST *parse_expr(DA_Token *toks, DA_Var *vars) {
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

static AST *parse_tokens(DA_Token *toks, DA_Var *vars) {
    AST *expr = parse_expr(toks, vars);
    if (DA_NEXT(*toks).kind != TOK_END) {
        return NULL;
    }
    return expr;
}

// TODO: This parses only a single line.
AST *parse_standard_expr(FILE *input, DA_Var *vars) {
    char input_buffer[LINE_MAX_STANDARD];
    if (!fgets(input_buffer, LINE_MAX_STANDARD, input)) { return NULL; }

    size_t input_len = strlen(input_buffer);
    while (input_buffer[input_len - 1] == '\n') {
        input_buffer[input_len - 1] = '\0';
        --input_len;
    }

    DA_Token toks = lex_string(input_buffer);
    AST *ast = parse_tokens(&toks, vars);

    printf("Input: \"%s\"\n", input_buffer);
    printf("Variables: ");
    printf("%s", vars->items[0]);
    for (size_t i = 1; i < vars->count; ++i) {
        printf(" %s", vars->items[i]);
    }
    putchar('\n');

    return ast;
}

#endif // PARSER_STANDARD_IMPL
