//
// Created by jamescoward on 25/10/2026.
//

#ifndef ATOMICA_SHUNTINGYARD_H
#define ATOMICA_SHUNTINGYARD_H

#include "Lexer/Token.h"
#include "Parser/Parser.h"

extern NodeArray node_array;;

NodeRes shunt(bool allow_trailing_parens);

bool is_valid_expr_start(TokenType type);

#endif //ATOMICA_SHUNTINGYARD_H
