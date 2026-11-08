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
        // token= l brace
        // lhs, rhs= extra array
        // extra= [..]Statements
    NT_VAR_DECL,
        // token= ident
        // lhs= type | NONE
        // rhs= value | NONE
    NT_CONST_DECL,
        // token= ident
        // lhs= type | NONE
        // rhs= value | NONE

    NT_IMPORT_STATEMENT,
        // token= import
        // lhs= string TOKEN
        // rhs= NONE

    NT_IF_EXPR,
        // token= if
        // lhs, rhs= extra
        // extra=   cond,
        //          output,
        //          else | NONE

    NT_ELSE_EXPR,
        // token= else
        // lhs= output
        // rhs= NONE

    NT_DO_WHILE_EXPR,
        // token= do
        // lhs= cond
        // rhs= output

    NT_WHILE_EXPR,
        // token= while
        // lhs= cond
        // rhs= output

    NT_FOR_EACH_EXPR,
        // token= for
        // lhs,rhs= extra
        // extra= element name TOKEN (if array name != NONE) OR expr index,
        //          array name TOKEN OR NONE
        //          with name TOKEN OR NONE
        //          output expression

    NT_FOR_EXPR,
        // token= for
        // lhs,rhs= extra
        // extra= setup expression | NONE,
        //          condition | NONE,
        //          iteration | NONE

    NT_LAMBDA,
        // token= lparen
        // lhs= labda header
        // rhs= body (compound statement)
    NT_LAMBDA_HEADER,
        // token= lparen
        // lhs, rhs= extra
        // extra= [..]params, return type
    NT_PARAM,
        // token: ident
        // lhs: type
        // rhs: default value | NONE

    NT_TYPED_IDENTIFIER,
        // token= ident
        // lhs= type
        // rhs= NONE

    NT_TYPE_ERRORABLE,
        // token= !
        // lhs= errors identifier TOKEN | NONE
        // rhs= return type index

    NT_TYPE_ARRAY_STATIC,
        // token= lbracket
        // lhs= inner type
        // rhs= size_expr
    NT_TYPE_ARRAY_DYNAMIC,
        // token= lbracket
        // lhs= inner type
        // rhs= NONE
    NT_TYPE_ARRAY_LINK,
        // token= lbracket
        // lhs= inner type
        // rhs= NONE
    NT_TYPE_ARRAY_POOL,
        // token= lbracket
        // lhs= inner type
        // rhs= NONE
    NT_TYPE_ARRAY_SLICE,
        // token= lbracket
        // lhs= inner type
        // rhs= NONE

    NT_TYPE_NULLABLE,
        // token= ?
        // lhs= inner type
        // rhs= NONE
    NT_TYPE_POINTER,
        // token= >
        // lhs= inner type
        // rhs= NONE
    NT_TYPE_MARKED,
        // token= '
        // lhs= inner type
        // rhs= NONE
    NT_TYPE_BASE, // n8, n1, byte, string
        // token= type
        // lhs= NONE
        // rhs= NONE
    NT_TYPE_LINK, // identifier to another type
        // token= identifier
        // lhs= NONE
        // rhs= NONE
    NT_TYPE_CONST,
        // token= const,
        // lhs= type|NONE
        // rhs= NONE


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


    NT_EXPR_LITERAL,
        // token= literal
        // lhs= NONE
        // rhs= NONE
    NT_EXPR_LITERAL_ARRAY,
        // token= [
        // lhs, rhs= extra
        // extra= [..]element expressions

    NT_EXPR_OPERATOR_BINARY,
        // token= operator
        // lhs= lhs
        // rhs=rhs
    NT_EXPR_OPERATOR_UNARY,
        // token= operator
        // lhs= operand
    NT_EXPR_VARIABLE,
        // token= ident
        // lhs= NONE
        // rhs= NONE
    NT_EXPR_FUNC_CALL,
        // token= ident,
        // lhs= extra start,
        // rhs= extra end
        // extra= [..]arg, NT_EXPR_*
    NT_EXPR_ARRAY_INDEX,
        // token= ident
        // lhs= index expr
        // rhs= NONE

    NT_EXPR_RETURN,
        // token= ret,
        // lhs= expr | NONE
        // rhs= NONE
    NT_EXPR_RETURN_ERROR,
        // token= ret
        // lhs= expr | NONE
        // rhs= NONE

    NT_EXPR_YIELD,
        // token= yield,
        // lhs= expr | NONE
        // rhs= NONE
    NT_EXPR_YIELD_ERROR,
        // token= yield,
        // lhs= expr | NONE
        // rhs= NONE

    NT_EXPR_BREAK,
        // token= break
        // lhs= expr | NONE
        // rhs= NONE
    NT_EXPR_BREAK_ERROR,
        // token= break
        // lhs= expr | NONE
        // rhs= NONE

    NT_EXPR_CONTINUE,
        // token= continue
        // lhs= NONE
        // rhs= NONE

    NT_EXPR_DEFER,
        // token= defer,
        // lhs= statement | NONE
        // rhs= NONE
    NT_EXPR_DEFER_ERROR,
        // token= defer,
        // lhs= statement | NONE
        // rhs= NONE
} NodeType;

typedef struct {
    NodeIndex cond;
    NodeIndex output;
    NodeIndex else_;
} NodeIf;
NodeIf get_node_if(NodeIndex ni);

typedef struct {
    size_t arg_count;
    size_t arg_start;
} NodeFuncCall;
NodeFuncCall get_node_func_call(NodeIndex ni);

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

#define TOKEN_AS_NODE_INDEX(token) (*(NodeIndex*)&token)
#define NODE_INDEX_AS_TOKEN(index) (*(Token*)&index)

#endif //ATOMICA_NODE_H
