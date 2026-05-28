#include <stdio.h>

#define DA_IMPL
#include "DA.h"

#define PARSER_IMPL
#include "parser.h"

int main(void) {
    //DA_Token toks = lex_string("a + b + c");
    //DA_Token toks = lex_string("a + bc - d * e");
    DA_Token toks = lex_string("(a)");
    //DA_Token toks = lex_string("(a)(b)");

    for (size_t i = 0; i < toks.count; ++i) {
        const Token tok = toks.items[i];
        printf("Token %ld: kind=%d chr=%c\n", i+1, tok.kind, tok.chr);
    }

    AST *ast = parse_tokens(&toks);
    AST_print(ast);

    return 0;
}
