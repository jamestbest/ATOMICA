//
// Created by jamescoward on 30/10/2026.
//

#ifndef ATOMICA_NODE_H
#define ATOMICA_NODE_H

#include <stdint.h>

#include "shared/Array.h"
#include "Error.h"
#include "Lexer/Token.h"

typedef uint32_t NodeIndex;
#define NODE_INDEX_NONE 0

typedef struct {
    ErrorCode errcode;
    NodeIndex index;
} NodeRes;

typedef enum {
    NT_ROOT,
    NT_COMPOUND_STATEMENT,   // { }
    NT_VAR_DECL,
        // token= ident
        // lhs= type | NONE
        // rhs= value | NONE
    NT_CONST_DECL,
        // token= ident
        // lhs= type | NONE
        // rhs= value | NONE

    NT_LAMBDA,
    NT_LAMBDA_HEADER,
    NT_PARAM,

    NT_TYPED_IDENTIFIER,
        // token= ident
        // lhs= type
        // rhs= NONE

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
    NT_TYPE_CONST, // token= const, lhs= type|NONE

    NT_TYPE_DATA,
        // token= data
        // lhs= parameterised fields | NONE
        // rhs= fields
    NT_DATA_FIELDS, // contains brace token & extra fields to NT_DATA_FIELD
        // token= brace
        // lhs= extra start
        // rhs= extra end
        // extra= [..](DATA_FIELD|DATA_VARIANT)
    NT_DATA_PARAMETERISED_FIELDS, // contains paren token & extra fields to NT_DATA_FIELD
        // token= paren
        // lhs= extra start
        // rhs= extra end
        // extra= [..]TYPED_IDENTIFIER

    NT_DATA_FIELD, // (struct like e.g. <name>: <type>) contains ident token & type in lhs
        // token= ident
        // lhs= type
        // rhs= default value | NONE
    NT_DATA_VARIANT, // <name>(<types>)= <default>
        // token= ident
        // lhs= start of extra
        // rhs= end of extra
        // extra= [..]types, default value | NONE

    NT_TYPE_MARKED,

    NT_EXPR_LITERAL, // token= literal
    NT_EXPR_OPERATOR_BINARY, // token= operator, lhs= lhs, rhs=rhs
    NT_EXPR_OPERATOR_UNARY, // token= operator, lhs= operand
    NT_EXPR_VARIABLE, // token= ident
    NT_EXPR_FUNC_CALL, // token= ident, lhs= extra start, rhs= extra end
    // extra contains args, NT_EXPR_*
    NT_EXPR_ARRAY_INDEX, // token= ident, lhs= index expr

    NT_EXPR_RETURN, // token= ret, lhs= expr
    NT_EXPR_BREAK,
    NT_EXPR_CONTINUE,
} NodeType;

ARRAY_PROTO(NodeType, NodeType)
ARRAY_PROTO(NodeIndex, NodeIndex);

typedef struct {
    TokenArray tokens;
    NodeTypeArray types;
    NodeIndexArray lhs;
    NodeIndexArray rhs;
} NodeArray;

extern NodeArray node_array;
extern NodeIndexArray extra_array;
extern TokenArray tokens;

const char* node_type_string(NodeType type);
void print_node(NodeIndex ni);

#endif //ATOMICA_NODE_H
