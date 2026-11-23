//
// Created by jamescoward on 14/10/2026.
//

#ifndef ATOMICA_LEXER_H
#define ATOMICA_LEXER_H
#include <stdint.h>

#include "Token.h"
#include "shared/Buffer.h"

typedef struct {
     int errcode;
     TokenArray tokens;
} LexRes;

LexRes lex(const char* file_path);
uint8_t* get_token_end_ptr(Token t);
uint8_t* get_token_start_ptr(Token t);
const char* lex_get_string(size_t buffer_index_start);

void set_lex_buffer(Buffer buff_);

#endif //ATOMICA_LEXER_H
