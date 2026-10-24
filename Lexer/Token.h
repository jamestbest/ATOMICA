//
// Created by jamescoward on 14/10/2026.
//

#ifndef ATOMICA_TOKEN_H
#define ATOMICA_TOKEN_H

#include <stdint.h>

#include "shared/Array.h"


typedef enum {
    TT_LBRACE,
    TT_RBRACE,
    TT_LBRACKET,
    TT_RBRACKET,
    TT_LPAREN,
    TT_RPAREN,

    TT_IDENTIFIER,

    TT_KEYWORD_BREAK,
    TT_KEYWORD_CONTINUE,
    TT_KEYWORD_DATA,
    TT_KEYWORD_DEFER,
    TT_KEYWORD_ELSE,
    TT_KEYWORD_ENTRY,
    TT_KEYWORD_FOR,
    TT_KEYWORD_IF,
    TT_KEYWORD_IMPORT,
    TT_KEYWORD_MARK,
    TT_KEYWORD_RET,
    TT_KEYWORD_UNMARK,
    TT_KEYWORD_WHILE,
    TT_KEYWORD_WITH,
    TT_KEYWORD_YIELD,

    TT_INTERNAL_TYPE_START,
    TT_TYPE_I1,
    TT_TYPE_I2,
    TT_TYPE_I4,
    TT_TYPE_I8,

    TT_TYPE_N1,
    TT_TYPE_N2,
    TT_TYPE_N4,
    TT_TYPE_N8,

    TT_TYPE_Q2,
    TT_TYPE_Q4,
    TT_TYPE_Q8,
    TT_TYPE_Q16,

    TT_TYPE_STRING,
    TT_TYPE_BOOL,
    TT_INTERNAL_TYPE_END,

    TT_BOOLEAN_TRUE,
    TT_BOOLEAN_FALSE,

    TT_NULL,

    TT_OPERATOR_PLUS, // +
    TT_OPERATOR_SUBTRACT, // -
    TT_OPERATOR_MULT, // *
    TT_OPERATOR_EQ, // ==
    TT_OPERATOR_NEQ, // !=
    TT_OPERATOR_LESS, // <
    TT_OPERATOR_MORE, // >
    TT_OPERATOR_LESS_EQ, // <=
    TT_OPERATOR_MORE_EQ, // >=

    TT_OPERATOR_ASSIGN, // =

    TT_OPERATOR_ASSIGN_MULT, // *=
    TT_OPERATOR_ASSIGN_PLUS, // +=
    TT_OPERATOR_ASSIGN_SUBTRACT, // -=


    TT_OPERATOR_PLUS_PLUS, // ++
    TT_OPERATOR_MINUS_MINUS, // --

    TT_COLON, // :
    TT_CARROT, // '
    TT_COMMA, // ,
    TT_TILDA, // ~

    TT_DOT, // .
    TT_EXCLAMATION, // !
    TT_QUESTION, // ?
    TT_HASH, // #

    TT_LIT_INT,
    TT_LIT_FLOAT,
    TT_LIT_STRING,

    TT_COMMENT,

    TT_DELIMITER, // ; or nl
    TT_EOTS,

    TT_IGNORE,
    TT_ERROR,

    TT_COUNT
} TokenType;

typedef struct {
    TokenType type: 8;
    uint32_t index: 24;
} Token;

ARRAY_PROTO(Token, Token)

const char* token_type_string(TokenType type);
void print_token(Token t, size_t line, size_t col);

#endif //ATOMICA_TOKEN_H
