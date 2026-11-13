//
// Created by jamescoward on 25/10/2026.
//

#ifndef ATOMICA_SHUNTINGYARD_H
#define ATOMICA_SHUNTINGYARD_H

#include "Lexer/Token.h"
#include "Parser/Parser.h"

NodeRes shunt(bool allow_trailing_parens);

bool is_valid_expr_start(MyTokenType type);
bool is_valid_qe_expr_start(MyTokenType type);

uint8_t get_prec(MyTokenType type);
bool is_prefix_operator(MyTokenType type);
bool is_postfix_operator(MyTokenType type);
bool is_unary_operator(MyTokenType type);
bool is_assign_operator(MyTokenType type);
bool is_right_assoc(MyTokenType type);
bool is_binary_operator(MyTokenType type);
bool is_ternary_operator(MyTokenType type);
bool is_operator(MyTokenType type);


#endif //ATOMICA_SHUNTINGYARD_H
