//
// Created by jamescoward on 23/10/2026.
//

#ifndef ATOMICA_PARSER_H
#define ATOMICA_PARSER_H

#include <stdint.h>

typedef uint32_t NodeIndex;
#define NODE_INDEX_NONE 0

typedef enum {
    EC_SUCCESS= 0,
    EC_FAIL= -1
} ErrorCode;

typedef struct {
    ErrorCode errcode;
    NodeIndex index;
} NodeRes;

NodeRes parse();

#endif //ATOMICA_PARSER_H
