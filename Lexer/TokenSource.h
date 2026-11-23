//
// Created by jamestbest on 11/19/26.
//

#ifndef ATOMICA_TOKENSOURCE_H
#define ATOMICA_TOKENSOURCE_H

#include <stdint.h>
#include <stddef.h>

#include "Token.h"

typedef struct {
    Token* data; // todo: make this uint32_t somehow
    uint32_t start_idx;
    uint32_t current_idx;
    uint32_t end_idx; // inclusive
    uint32_t start_line;
    uint32_t max_tokens;
} MyTokenSource;

MyTokenSource clone_source(MyTokenSource source, size_t to, size_t from, size_t start_line);
void resize_source(MyTokenSource* source, size_t new_size);
void combine_source(MyTokenSource* source, MyTokenSource addition, size_t source_insert_idx, size_t overwrite_amount);

void print_token(Token t, size_t line, size_t col, MyTokenSource source);
void print_all_tokens(MyTokenSource source);

#endif //ATOMICA_TOKENSOURCE_H
