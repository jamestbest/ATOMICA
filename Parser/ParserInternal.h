//
// Created by jamescoward on 24/10/2026.
//

#ifndef ATOMICA_PARSERINTERNAL_H
#define ATOMICA_PARSERINTERNAL_H

#include "Lexer/Token.h"

#include "Parser/SharedParsing.h"

#define EXPECT(type) expect(type, NULL)

ARRAY_ADD(NodeType, NodeType)
ARRAY_ADD(NodeIndex, NodeIndex)

static NodeRes parse_top_level_statement();

typedef struct {
    Token token;
    NodeIndex type;
    NodeIndex value;
} VarDeclNode;

static VarDeclNode var_decl_node_get(NodeIndex index);

static NodeRes parse_lambda_header();
static NodeRes parse_expr(bool allow_trailing_parens);

static void destroy_node_array();
static void setup_node_array();

#endif //ATOMICA_PARSERINTERNAL_H
