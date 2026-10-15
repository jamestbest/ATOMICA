//
// Created by jamescoward on 14/10/2026.
//

#ifndef ATOMICA_LEXER_H
#define ATOMICA_LEXER_H
#include <stdint.h>

typedef struct {
     uint8_t* data;
     size_t size;
     size_t pos;
} Buffer;

int lex(const char* file_path);


#endif //ATOMICA_LEXER_H
