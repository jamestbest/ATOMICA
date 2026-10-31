//
// Created by jamescoward on 23/10/2026.
//

#ifndef ATOMICA_PARSERR_H
#define ATOMICA_PARSERR_H

#include "Lexer/Token.h"
#include "Parser/Node.h"

#define NODE_SUCC(ni) ((NodeRes) {.errcode= EC_SUCCESS, .index= ni})
#define NODE_FAIL(code) ((NodeRes) {.errcode= code, .index= -1})

NodeRes parse_error(const char* message, ...);
NodeRes unexpected(TokenType expected_type, Token received, char* message, ...);

#endif //ATOMICA_PARSERR_H
