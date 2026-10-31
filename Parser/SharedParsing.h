//
// Created by jamescoward on 25/10/2026.
//

#ifndef ATOMICA_SHAREDPARSING_H
#define ATOMICA_SHAREDPARSING_H

#include "Parser.h"
#include "Lexer/Token.h"

void eat_all_delimiters();
Token current();
Token consume();
Token peek();
Token peer(size_t offset);
bool check(TokenType type);
bool expect(TokenType type, Token* out);
bool between(TokenType type, TokenType start_exclusive, TokenType end_exclusive);

#define SET_LHS(elem, index) (NodeIndex_array_set(&node_array.lhs, elem, index))
#define SET_RHS(elem, index) (NodeIndex_array_set(&node_array.rhs, elem, index))
#define SET_TOKEN(elem, index) (NodeIndex_array_set(&node_array.tokens, elem, index))
#define SET_TYPE(elem, index) (NodeType_array_set(&node_array.types, elem, index))

#define GET_LHS(index) (NodeIndex_array_get(&node_array.lhs, index))
#define GET_RHS(index) (NodeIndex_array_get(&node_array.rhs, index))
#define GET_TOKEN(index) (Token_array_get(&node_array.tokens, index))
#define GET_TYPE(index) (NodeType_array_get(&node_array.types, index))

#define PROP_FAIL(res) { NodeRes r= res; if (r.errcode != EC_SUCCESS) { return NODE_FAIL(r.errcode); }}

NodeIndex node_create(NodeType type, Token token);

NodeRes parse_identifier();
NodeRes parse_ret();
NodeRes parse_break();
NodeRes parse_continue();
NodeRes parse_for();
NodeRes parse_while();
NodeRes parse_do();
NodeRes parse_if();
NodeRes parse_brace();

NodeRes parse_type();


#endif //ATOMICA_SHAREDPARSING_H
