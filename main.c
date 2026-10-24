#include <stdio.h>

#include "Lexer/Lexer.h"
#include "Lexer/Token.h"
#include "Parser/Parser.h"

TokenArray tokens;

int main(void) {
    const int lex_res= lex("C:\\Users\\jamescoward\\CLionProjects\\ATOMICA\\atom_examples\\900.atm");
    printf("LEXRES: %d\n", lex_res);

    const NodeRes parse_res= parse();
    printf("PARSERES: %d\n", parse_res.errcode);

    return 0;
}
