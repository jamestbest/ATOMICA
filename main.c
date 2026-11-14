#include <stdio.h>

#include "Gluon/Gluon.h"
#include "Lexer/Lexer.h"
#include "Lexer/Token.h"
#include "Logger/Logger.h"
#include "Parser/Parser.h"

int main(void) {
    logger_init();

    return orchestrate("/home/jamestbest/ATOMICA/atom_examples/900.atm");

    // const LexRes lex_res= lex();
    // logger_printf("LEXRES: %d\n", lex_res.errcode);
    //
    // if (lex_res.errcode != EC_SUCCESS) {
    //     return lex_res.errcode;
    // }
    //
    // const NodeRes parse_res= parse();
    // logger_printf("PARSERES: %d\n", parse_res.errcode);

    return 0;
}
