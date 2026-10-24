//
// Created by jamescoward on 14/10/2026.
//

#include "Token.h"

#include <stdio.h>

#include "Lexer.h"

static const char* TOKEN_TYPE_STRINGS[TT_COUNT]= {
    [TT_LBRACE]= "LBRACE",
    [TT_RBRACE]= "RBRACE",
    [TT_LBRACKET]= "LBRACKET",
    [TT_RBRACKET]= "RBRACKET",
    [TT_LPAREN]= "LPAREN",
    [TT_RPAREN]= "RPAREN",
    [TT_IDENTIFIER]= "IDENTIFIER",
    [TT_KEYWORD_BREAK]= "KEYWORD_BREAK",
    [TT_KEYWORD_CONTINUE]= "KEYWORD_CONTINUE",
    [TT_KEYWORD_DATA]= "KEYWORD_DATA",
    [TT_KEYWORD_DEFER]= "KEYWORD_DEFER",
    [TT_KEYWORD_ELSE]= "KEYWORD_ELSE",
    [TT_KEYWORD_ENTRY]= "KEYWORD_ENTRY",
    [TT_KEYWORD_FOR]= "KEYWORD_FOR",
    [TT_KEYWORD_IF]= "KEYWORD_IF",
    [TT_KEYWORD_IMPORT]= "KEYWORD_IMPORT",
    [TT_KEYWORD_MARK]= "KEYWORD_MARK",
    [TT_KEYWORD_RET]= "KEYWORD_RET",
    [TT_KEYWORD_UNMARK]= "KEYWORD_UNMARK",
    [TT_KEYWORD_WHILE]= "KEYWORD_WHILE",
    [TT_KEYWORD_WITH]= "KEYWORD_WITH",
    [TT_KEYWORD_YIELD]= "KEYWORD_YIELD",
    [TT_TYPE_I1]= "TYPE_I1",
    [TT_TYPE_I2]= "TYPE_I2",
    [TT_TYPE_I4]= "TYPE_I4",
    [TT_TYPE_I8]= "TYPE_I8",
    [TT_TYPE_N1]= "TYPE_N1",
    [TT_TYPE_N2]= "TYPE_N2",
    [TT_TYPE_N4]= "TYPE_N4",
    [TT_TYPE_N8]= "TYPE_N8",
    [TT_TYPE_Q2]= "TYPE_Q2",
    [TT_TYPE_Q4]= "TYPE_Q4",
    [TT_TYPE_Q8]= "TYPE_Q8",
    [TT_TYPE_Q16]= "TYPE_Q16",
    [TT_TYPE_STRING]= "TYPE_STRING",
    [TT_TYPE_BOOL]= "TYPE_BOOL",
    [TT_BOOLEAN_TRUE]= "BOOLEAN_TRUE",
    [TT_BOOLEAN_FALSE]= "BOOLEAN_FALSE",
    [TT_NULL]= "NULL",
    [TT_OPERATOR_PLUS]= "OPERATOR_PLUS",
    [TT_OPERATOR_SUBTRACT]= "OPERATOR_SUBTRACT",
    [TT_OPERATOR_MULT]= "OPERATOR_MULT",
    [TT_OPERATOR_EQ]= "OPERATOR_EQ",
    [TT_OPERATOR_NEQ]= "OPERATOR_NEQ",
    [TT_OPERATOR_LESS]= "OPERATOR_LESS",
    [TT_OPERATOR_MORE]= "OPERATOR_MORE",
    [TT_OPERATOR_LESS_EQ]= "OPERATOR_LESS_EQ",
    [TT_OPERATOR_MORE_EQ]= "OPERATOR_MORE_EQ",
    [TT_OPERATOR_ASSIGN]= "OPERATOR_ASSIGN",
    [TT_OPERATOR_ASSIGN_MULT]= "OPERATOR_ASSIGN_MULT",
    [TT_OPERATOR_ASSIGN_PLUS]= "OPERATOR_ASSIGN_PLUS",
    [TT_OPERATOR_ASSIGN_SUBTRACT]= "OPERATOR_ASSIGN_SUBTRACT",
    [TT_OPERATOR_PLUS_PLUS]= "OPERATOR_PLUS_PLUS",
    [TT_OPERATOR_MINUS_MINUS]= "OPERATOR_MINUS_MINUS",
    [TT_COLON]= "COLON",
    [TT_CARROT]= "CARROT",
    [TT_COMMA]= "COMMA",
    [TT_TILDA]= "TILDA",
    [TT_DOT]= "DOT",
    [TT_EXCLAMATION]= "EXCLAMATION",
    [TT_QUESTION]= "QUESTION",
    [TT_HASH]= "HASH",
    [TT_LIT_INT]= "LIT_INT",
    [TT_LIT_FLOAT]= "LIT_FLOAT",
    [TT_LIT_STRING]= "LIT_STRING",
    [TT_COMMENT]= "COMMENT",
    [TT_DELIMITER]= "DELIMITER",
    [TT_EOTS]= "EOTS",
    [TT_IGNORE]= "IGNORE",
    [TT_ERROR]= "ERROR",
};

const char* token_type_string(const TokenType type) {
    if (type > TT_COUNT) return "<<UNKNOWN TYPE>>";

    return TOKEN_TYPE_STRINGS[type];
}

// line and col can be -1 for unknown
// todo: find the col and line info if it's not provided
void print_token(const Token t, size_t line, size_t col) {
    printf("<TOKEN:%s@", token_type_string(t.type));
    if (line == -1 || col == -1) {
        printf("?:?> ");
    } else {
        printf("%zu:%zu> ", line, col);
    }

    uint8_t* start= get_token_start_ptr(t);
    uint8_t* end= get_token_end_ptr(t);

    if (*start != '\n') {
        uint8_t save= *end;
        *end= '\0';
        printf("%s", (const char*)start);
        *end= save;
    }
}
