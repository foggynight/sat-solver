#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define AST_IMPL
#include "AST.h"

#define DA_IMPL
#include "DA.h"

#define EVAL_IMPL
#include "eval.h"

#define PARSER_IMPL
#include "parser.h"

int main(void) {
    //DA_Token toks = lex_string("a + b + c");
    //DA_Token toks = lex_string("a * b + c * d");
    //DA_Token toks = lex_string("a + b * (c + d)");
    //DA_Token toks = lex_string("a + (b * c) + d");
    //DA_Token toks = lex_string("a + -(bc -- d * e)");
    //DA_Token toks = lex_string("a + bc + d * e");
    //DA_Token toks = lex_string("(a)");
    //DA_Token toks = lex_string("-a");
    DA_Token toks = lex_string("(a)(b)(c)");
    //DA_Token toks = lex_string("a + b");
    //DA_Token toks = lex_string("a + b c");
    //DA_Token toks = lex_string("-(-a * -b)");
    //DA_Token toks = lex_string("(a + -b)(a + c)(b + c)(a + b + c)");

    //for (size_t i = 0; i < toks.count; ++i) {
    //    const Token tok = toks.items[i];
    //    printf("Token %ld: kind=%d chr=%c\n", i+1, tok.kind, tok.chr);
    //}

    DA_char vars = {0};
    AST *ast = parse_tokens(&toks, &vars);

    printf("AST: ");
    AST_print(ast);
    putchar('\n');

    printf("vars:");
    for (size_t i = 0; i < vars.count; ++i) {
        printf(" %c", vars.items[i]);
    }
    putchar('\n');

    Bind bind_a = { 'a', 1 };
    Bind bind_b = { 'b', 1 };
    Bind bind_c = { 'c', 1 };
    DA_Bind binds = {0};
    DA_APPEND(binds, bind_a);
    DA_APPEND(binds, bind_b);
    DA_APPEND(binds, bind_c);

    AST *result = eval_ast_binds(ast, &binds);
    if (!result) {
        fprintf(stderr, "error: failed to eval AST with binds\n");
        exit(1);
    }

    bool result_bool;
    switch (result->kind) {
    case AST_TRUE: result_bool = true; break;
    case AST_FALSE: result_bool = false; break;
    default:
        fprintf(stderr, "error: invalid result from eval\n");
        exit(1);
    }

    printf("binds:");
    for (size_t i = 0; i < binds.count; ++i) {
        Bind bind = binds.items[i];
        printf(" (%c %d)", bind.var, bind.val);
    }
    putchar('\n');

    printf("result: %d\n", result_bool);

    return 0;
}
