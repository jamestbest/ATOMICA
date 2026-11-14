//
// Created by jamescoward on 23/10/2026.
//

#ifndef ATOMICA_PARSER_H
#define ATOMICA_PARSER_H

#include "Lexer/Token.h"
#include "shared/Array.h"
#include "Node.h"

bool is_valid_type_start(MyTokenType type);

NodeRes parse(MyTokenSource source);

#endif //ATOMICA_PARSER_H
