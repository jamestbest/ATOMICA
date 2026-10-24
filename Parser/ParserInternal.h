//
// Created by jamescoward on 24/10/2026.
//

#ifndef ATOMICA_PARSERINTERNAL_H
#define ATOMICA_PARSERINTERNAL_H

#include "Lexer/Token.h"

typedef enum {
    NT_ROOT,
    NT_COMPOUND_STATEMENT,   // { }
    NT_VAR_DECL,

    NT_LAMBDA,
    NT_LAMBDA_HEADER,
    NT_PARAM,

    NT_TYPE_ERRORABLE,

    NT_TYPE_ARRAY_STATIC,
    NT_TYPE_ARRAY_DYNAMIC,
    NT_TYPE_ARRAY_LINK,
    NT_TYPE_ARRAY_POOL,
    NT_TYPE_ARRAY_SLICE,

    NT_TYPE_NULLABLE,
    NT_TYPE_POINTER,
    NT_TYPE_BASE, // n8, n1, byte, string
    NT_TYPE_LINK, // identifier to another type

    NT_TYPE_MARKED,
} NodeType;

static NodeRes parse_type();

static Token current();
static Token consume();
static Token peek();
static Token peer(size_t offset);
static bool  expect(TokenType type, Token* out);
static bool check(TokenType type);
static bool between(TokenType type, TokenType start_exclusive, TokenType end_exclusive);
#define EXPECT(type) expect(type, NULL)

#define PROP_FAIL(res) { NodeRes r= res; if (r.errcode != EC_SUCCESS) { return NODE_FAIL(r.errcode); }}

ARRAY_PROTO_ADD(NodeType, NodeType)
ARRAY_PROTO_ADD(NodeIndex, NodeIndex)

static NodeRes parse_top_level_statement();

typedef struct {
    Token token;
    NodeIndex type;
    NodeIndex value;
} VarDeclNode;

static VarDeclNode var_decl_node_get(NodeIndex index);
static NodeIndex var_decl_node_create(Token ident, NodeIndex type, NodeIndex value);
static NodeIndex node_create(NodeType type, Token token);

NodeRes parse_lambda_header();

static void destroy_node_array();
static void setup_node_array();

#endif //ATOMICA_PARSERINTERNAL_H
