#include "parser_infix.h"

#include <ctype.h>

#include "AST.h"
#include "DA.h"
#include "util.h"

#define LINE_MAX_INFIX 1048576

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
        unary->kind = AST_OP;
        unary->token = DA_NEXT(*toks);
        DA_DEQUE(*toks);

        AST *right = parse_fact(toks, vars);
        if (!right) {
            AST_free(unary);
            return NULL;
        }

        AST_append(unary, right);
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
    AST *term, *fact;

    fact = parse_fact(toks, vars);
    if (fact == NULL) { return NULL; }
    if (DA_NEXT(*toks).kind != TOK_VAR
        && DA_NEXT(*toks).kind != TOK_MINUS
        && DA_NEXT(*toks).kind != TOK_STAR
        && DA_NEXT(*toks).kind != TOK_PAREN_L)
    {
        return fact;
    }
    if (DA_NEXT(*toks).kind == TOK_STAR) { DA_DEQUE(*toks); }

    term = AST_make();
    term->kind = AST_OP;
    term->token = Token_star();
    AST_append(term, fact);

    while ((fact = parse_fact(toks, vars)) != NULL) {
        AST_append(term, fact);
        if (DA_NEXT(*toks).kind != TOK_VAR
            && DA_NEXT(*toks).kind != TOK_MINUS
            && DA_NEXT(*toks).kind != TOK_STAR
            && DA_NEXT(*toks).kind != TOK_PAREN_L)
        {
            break;
        }
        if (DA_NEXT(*toks).kind == TOK_STAR) { DA_DEQUE(*toks); }
    }

    return (fact == NULL) ? NULL : term;
}

static AST *parse_expr(DA_Token *toks, DA_Var *vars) {
    AST *expr, *term;

    term = parse_term(toks, vars);
    if (term == NULL) { return NULL; }
    if (DA_NEXT(*toks).kind != TOK_PLUS) { return term; }
    DA_DEQUE(*toks);

    expr = AST_make();
    expr->kind = AST_OP;
    expr->token = Token_plus();
    AST_append(expr, term);

    while ((term = parse_term(toks, vars)) != NULL) {
        AST_append(expr, term);
        if (DA_NEXT(*toks).kind != TOK_PLUS) { break; }
        DA_DEQUE(*toks);
    }

    return (term == NULL) ? NULL : expr;
}

static AST *parse_tokens(DA_Token *toks, DA_Var *vars) {
    AST *expr = parse_expr(toks, vars);
    if (DA_NEXT(*toks).kind != TOK_END) {
        return NULL;
    }
    return expr;
}

// TODO: This parses only a single line.
AST *parse_expr_infix(FILE *input, DA_Var *vars) {
    char input_buffer[LINE_MAX_INFIX];
    if (!fgets(input_buffer, LINE_MAX_INFIX, input)) { return NULL; }
    printf("Input: \"%s\"\n", input_buffer);

    size_t input_len = strlen(input_buffer);
    while (input_buffer[input_len - 1] == '\n') {
        input_buffer[input_len - 1] = '\0';
        --input_len;
    }

    DA_Token toks = lex_string(input_buffer);
    AST *ast = parse_tokens(&toks, vars);

    return ast;
}
