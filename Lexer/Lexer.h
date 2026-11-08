//
// Created by jamescoward on 14/10/2026.
//

#ifndef ATOMICA_LEXER_H
#define ATOMICA_LEXER_H
#include <stdint.h>

#include "Token.h"

typedef struct {
     uint8_t* data;
     size_t size;
     size_t pos;
} Buffer;

int lex(const char* file_path);
uint8_t* get_token_end_ptr(Token t);
uint8_t* get_token_start_ptr(Token t);

#endif //ATOMICA_LEXER_H
