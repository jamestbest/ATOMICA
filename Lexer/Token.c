//
// Created by jamescoward on 14/10/2026.
//

#include "Token.h"

#include <stdio.h>

#include "Lexer.h"
#include "shared/String.h"

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
    [TT_KEYWORD_CONST]= "KEYWORD_CONST",
    [TT_KEYWORD_DATA]= "KEYWORD_DATA",
    [TT_KEYWORD_DEFER]= "KEYWORD_DEFER",
    [TT_KEYWORD_DO]= "KEYWORD_DO",
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
    [TT_OPERATOR_AMBIGUOUS_PLUS]= "OPERATOR_PLUS",
    [TT_OPERATOR_AMBIGUOUS_SUBTRACT]= "OPERATOR_SUBTRACT",
    [TT_OPERATOR_MULT]= "OPERATOR_MULT",
    [TT_OPERATOR_DIV]= "OPERATOR_DIV",
    [TT_OPERATOR_MOD]= "OPERATOR_MOD",
    [TT_OPERATOR_SHIFT_LEFT]= "OPERATOR_SHIFT_LEFT",
    [TT_OPERATOR_AMBIGUOUS_SHIFT_RIGHT]= "OPERATOR_SHIFT_RIGHT",
    [TT_OPERATOR_LOGICAL_AND]= "OPERATOR_LOGICAL_AND",
    [TT_OPERATOR_LOGICAL_OR]= "OPERATOR_LOGICAL_OR",
    [TT_OPERATOR_LOGICAL_XOR]= "OPERATOR_LOGICAL_XOR",
    [TT_OPERATOR_EQ]= "OPERATOR_EQ",
    [TT_OPERATOR_NEQ]= "OPERATOR_NEQ",
    [TT_OPERATOR_LESS]= "OPERATOR_LESS",
    [TT_OPERATOR_AMBIGUOUS_MORE]= "OPERATOR_MORE",
    [TT_OPERATOR_LESS_EQ]= "OPERATOR_LESS_EQ",
    [TT_OPERATOR_MORE_EQ]= "OPERATOR_MORE_EQ",
    [TT_OPERATOR_ASSIGN]= "OPERATOR_ASSIGN",
    [TT_OPERATOR_ASSIGN_MULT]= "OPERATOR_ASSIGN_MULT",
    [TT_OPERATOR_ASSIGN_DIV]= "OPERATOR_ASSIGN_DIV",
    [TT_OPERATOR_ASSIGN_MOD]= "OPERATOR_ASSIGN_MOD",
    [TT_OPERATOR_ASSIGN_PLUS]= "OPERATOR_ASSIGN_PLUS",
    [TT_OPERATOR_ASSIGN_SUBTRACT]= "OPERATOR_ASSIGN_SUBTRACT",
    [TT_OPERATOR_BITWISE_AND]= "OPERATOR_BITWISE_AND",
    [TT_OPERATOR_AMBIGUOUS_BITWISE_OR]= "OPERATOR_BITWISE_OR",
    [TT_OPERATOR_BITWISE_XOR]= "OPERATOR_BITWISE_XOR",
    [TT_OPERATOR_AMBIGUOUS_PLUS_PLUS]= "OPERATOR_PLUS_PLUS",
    [TT_OPERATOR_AMBIGUOUS_MINUS_MINUS]= "OPERATOR_MINUS_MINUS",
    [TT_OPERATOR_PREFIX_PLUS_PLUS]= "PREFIX_PLUS_PLUS",
    [TT_OPERATOR_PREFIX_MINUS_MINUS]= "PREFIX_MINUS_MINUS",
    [TT_OPERATOR_POSTFIX_PLUS_PLUS]= "POSTFIX_PLUS_PLUS",
    [TT_OPERATOR_POSTFIX_MINUS_MINUS]= "POSTFIX_MINUS_MINUS",
    [TT_OPERATOR_PREFIX_PLUS]= "PREFIX_PLUS",
    [TT_OPERATOR_PREFIX_SUBTRACT]= "PREFIX_SUBTRACT",
    [TT_OPERATOR_PREFIX_ADDRESS]= "PREFIX_ADDRESS",
    [TT_OPERATOR_PREFIX_EXCLAMATION]= "PREFIX_EXCLAMATION",
    [TT_OPERATOR_POSTFIX_DOT]= "POSTFIX_DOT",
    [TT_OPERATOR_POSTFIX_QUESTION]= "POSTFIX_QUESTION",
    [TT_OPERATOR_POSTFIX_EXCLAMATION]= "POSTFIX_EXCLAMATION",
    [TT_COLON]= "COLON",
    [TT_OPERATOR_CARROT]= "CARROT",
    [TT_COMMA]= "COMMA",
    [TT_OPERATOR_CAST]= "CAST(~)",
    [TT_OPERATOR_AMBIGUOUS_DOT]= "DOT",
    [TT_OPERATOR_AMBIGUOUS_EXCLAMATION]= "EXCLAMATION",
    [TT_OPERATOR_AMBIGUOUS_QUESTION]= "QUESTION",
    [TT_OPERATOR_BINARY_PLUS]= "PLUS (BIN)",
    [TT_OPERATOR_BINARY_SUBTRACT]= "SUBTRACT (BIN)",
    [TT_OPERATOR_BINARY_MORE]= "MORE (BIN)",
    [TT_OPERATOR_BINARY_DOT]= "DOT (BIN)",
    [TT_OPERATOR_BINARY_QUESTION]= "QUESTION (BIN)",
    [TT_OPERATOR_BINARY_EXCLAMATION]= "EXCLAMATION (BIN)",
    [TT_OPERATOR_BINARY_SHIFT_RIGHT]= "SHIFT_RIGHT (BIN)",
    [TT_HASH]= "HASH",
    [TT_LIT_INT]= "LIT_INT",
    [TT_LIT_FLOAT]= "LIT_FLOAT",
    [TT_LIT_STRING]= "LIT_STRING",
    [TT_COMMENT]= "COMMENT",
    [TT_DELIMITER]= "DELIMITER",
    [TT_NEWLINE]= "NEWLINE",
    [TT_EOTS]= "EOTS",
    [TT_IGNORE]= "IGNORE",
    [TT_ERROR]= "ERROR",
};

const char* token_type_string(const TokenType type) {
    if (type > TT_COUNT) return "<<UNKNOWN TYPE>>";

    return TOKEN_TYPE_STRINGS[type];
}

static uint32_t cached_line= 0;
static uint32_t cached_line_token_index= 0; // index into tokens

static void get_line_and_col_info(
    const uint32_t target_buffer_index,
    size_t *line_out,
    size_t *col_out,
    const TokenArray* tokens
) {
    size_t start_buffer_index= Token_array_get(tokens, cached_line_token_index).buffer_index;
    size_t start_token_index= cached_line_token_index;
    size_t start_line= cached_line;
    if (target_buffer_index < start_buffer_index) {
        start_buffer_index= 0;
        start_line= 1;
        start_token_index= 0;
    }

    size_t c_token_index= start_token_index;
    Token c_tok;
    size_t c_line= start_line;
    size_t c_line_buffer_index= start_buffer_index;
    while (c_tok= Token_array_get(tokens, c_token_index),
        c_tok.buffer_index < target_buffer_index
    ) {
        c_token_index++;

        if (c_tok.type == TT_NEWLINE) {
            c_line++;
            c_line_buffer_index= c_tok.buffer_index + 1;
        }
    }

    *line_out= c_line + 1;
    *col_out= c_tok.buffer_index - c_line_buffer_index + 1;
}

void print_token_value(const Token t) {
    if (t.type == TT_EOTS) return;

    const uint8_t* start= get_token_start_ptr(t);
    uint8_t* end= get_token_end_ptr(t);

    if (*start != '\n') {
        const uint8_t save= *end;
        *end= '\0';
        printf("%s", (const char*)start);
        *end= save;
    }
}

// line and col can be -1 for unknown
void print_token(const Token t, size_t line, size_t col, const TokenArray* tokens) {
    printf("<TOKEN:%s@", token_type_string(t.type));
    if (line == -1 || col == -1) {
        get_line_and_col_info(t.buffer_index, &line, &col, tokens);
    }

    printf("%zu:%zu> ", line, col);
    print_token_value(t);
}

void print_all_tokens(const TokenArray* tokens) {
    for (int i = 0; i < tokens->pos; ++i) {
        print_token(Token_array_get(tokens, i), -1, -1, tokens);
        newline();
    }
}

