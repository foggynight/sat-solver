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

    DA_Bind binds = binds_zero(&vars);
    for (size_t i = 0; i < 8; ++i) {
        printf("\nbinds:");
        for (size_t i = 0; i < binds.count; ++i) {
            Bind bind = binds.items[i];
            printf(" (%c %d)", bind.var, bind.val);
        }
        putchar('\n');

        AST *result = eval_ast_binds(ast, &binds);
        printf("result: %d\n", AST_to_bool(result));

        binds_inc(&binds);
    }

    return 0;
}
